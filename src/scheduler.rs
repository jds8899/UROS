use core::ptr;
use core::ffi;
use core::mem;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;
use crate::x86arch;
use crate::pcbs::Pcb;
use crate::pcbs;
use crate::stacks;

extern "C" {
    #[no_mangle]
    fn _kmalloc(size:u64) -> usize;
}

pub const NUM_PROC: u8 = 8;
const QUANTUM_STD: u8 = 5;

pub struct Scheduler {
    proc_stat: &'static mut ProcStatus,
    procs: &'static mut Procs,
    q: &'static mut ProcSched,
    in_use: u8,
    current: u8,
    sched_ptr: u8,
}

struct ProcStatus {
    data: [u8; (NUM_PROC as usize)],
}

struct ProcSched {
    data: [i8; (NUM_PROC as usize)],
}

struct Procs {
    data: [Pcb; (NUM_PROC as usize)],
}

impl Scheduler {
    pub fn _add_proc(&mut self, cxt: u64, stk:u64, event:u32, extst:u32,
                     pid:u16, ppid:u16, children:u16) -> usize {
        if self.in_use == 7 {
            return 9;
        }
        let mut next = 0 as usize;
        if self.in_use == 0 {
            next = 0;

            self.procs.data[0].spot = 0;
            self.proc_stat.data[0]  = 1;
        }
        else if self.in_use == NUM_PROC {
            //None
            return 9;
        }
        else {
            for i in 1..NUM_PROC {
                if self.proc_stat.data[i as usize] == 0 {
                    next = i as usize;
                    self.procs.data[next].spot = next as i8;
                    self.proc_stat.data[next]  = 1;
                    break;
                }
            }
        }
        self.in_use += 1;

        unsafe {
            self.procs.data[next].cxt        = &mut *(cxt as *mut pcbs::Context);

            self.procs.data[next].stack      = &mut *(stk as *mut stacks::StkBuffer);
        }

        self.procs.data[next].event      = event;
        self.procs.data[next].exitstatus = extst;

        self.procs.data[next].pid        = pid;
        self.procs.data[next].ppid       = ppid;
        self.procs.data[next].children   = children;
        self.procs.data[next].state      = pcbs::ST_READY;

        return next;
        //self.procs.data[next].state = pcbs::e_states::ST_READY;
    }

    pub fn _schedule(&mut self, ind:i8) {
        if self.q.data[self.sched_ptr as usize] == -1 &&
            self.procs.data[ind as usize].state != pcbs::ST_WAITING &&
            self.procs.data[ind as usize].state != pcbs::ST_ZOMBIE {

            self.q.data[self.sched_ptr as usize] = ind;
            self.sched_ptr = (self.sched_ptr + 1) % NUM_PROC;
        }
    }

    pub fn _dispatch(&mut self) {
        self.q.data[self.current as usize] = -1;
        self.current = (self.current + 1) % NUM_PROC;

        let ind  = self.q.data[self.current as usize] as usize;
        self.procs.data[ind].state = pcbs::ST_RUNNING;
        self.procs.data[ind].ticks = QUANTUM_STD;
    }

    pub fn get_curr_cxt(&mut self) -> u64 {
        let ind  = self.q.data[self.current as usize] as usize;
        let curr = &mut *(self.procs.data[ind].cxt) as *mut pcbs::Context;
        return curr as u64;
    }

    pub fn get_curr(&mut self) -> u64 {
        let ind  = self.q.data[self.current as usize] as usize;
        let curr = &mut (self.procs.data[ind]) as *mut Pcb;
        return curr as u64;
    }

    pub fn set_curr_cxt(&mut self, rsp:u64) {
        let ind  = self.q.data[self.current as usize] as usize;
        self.procs.data[ind].cxt = unsafe { &mut *(rsp as *mut pcbs::Context) };
    }

    pub fn dump_curr(&mut self) {
        let curr = &self.procs.data[self.current as usize];
        println!("cxt: {:p}",curr.cxt);
        println!("rsp: {:x}",curr.cxt.rsp);
        println!("stk: {:p}",curr.stack);
        println!("pid: {:x}",curr.pid);
        println!("ppid: {:x}",curr.ppid);
        println!("children: {:x}",curr.children);
    }

    pub fn _clear_sched(&mut self) {
        for i in 0..NUM_PROC {
            self.q.data[i as usize] = -1;
        }
    }

    pub fn get_in_use(&mut self) -> u8 {
        return self.in_use;
    }

    pub fn bite(&mut self, ind: i8) {

        if self.procs.data[ind as usize].children > 0 {
            for i in 0..NUM_PROC {
                if i as i8 != ind {
                    if self.procs.data[ind as usize].ppid == ind as u16 {
                        self.procs.data[i as usize].ppid                 = 0;
                        self.procs.data[ind as usize].children -= 1;
                        self.procs.data[0].children            += 1;
                    }
                }
            }
        }

        let mut parent = 0 as usize;
        for i in 0..NUM_PROC {
            if self.procs.data[i as usize].pid == self.procs.data[ind as usize].ppid {
                parent = i as usize;
            }
        }
        if self.procs.data[parent].state == pcbs::ST_WAITING {
            //println!("p found");
            self.procs.data[parent].cxt.rax   = self.procs.data[ind as usize].pid as u64;
            self.procs.data[parent].state     = pcbs::ST_READY;
            self.procs.data[parent].children -= 1;
            let sched_spot = self.procs.data[parent].spot;
            self._schedule(sched_spot);
            self.in_use -= 1;
            self.proc_stat.data[ind as usize] = 0;
        }
        else {
            //println!("p not found");
            self.procs.data[ind as usize].state = pcbs::ST_ZOMBIE;
        }

    }

    pub fn find_zombie(&mut self, ppid: u16) -> i8 {
        let mut ret = 9 as i8;
        for i in 0..NUM_PROC {
            //println!("proc ppid {} ppid {} state {}", self.procs.data[i as usize].ppid, ppid, self.procs.data[i as usize].state);
            if self.procs.data[i as usize].ppid == ppid &&
                self.procs.data[i as usize].state == pcbs::ST_ZOMBIE {
                    ret = i as i8;
                    break;
            }
        }
        return ret
    }

    pub fn get_pid(&mut self, ind:i8) -> u16 {
        return self.procs.data[ind as usize].pid;
    }

    pub fn rem_pcb(&mut self, ind:i8) {
        self.proc_stat.data[ind as usize] = 0;
        self.procs.data[ind as usize].state = pcbs::ST_UNUSED;
        self.in_use -= 1;
    }
}

lazy_static! {
    pub static ref SCHED: Mutex<Scheduler> = Mutex::new(Scheduler {
        proc_stat: unsafe { &mut *(_kmalloc(NUM_PROC as u64) as *mut ProcStatus) },
        procs: unsafe { &mut *(_kmalloc((NUM_PROC as u64) * (mem::size_of::<Pcb>() as u64)) as *mut Procs) },
        q: unsafe { &mut *(_kmalloc(NUM_PROC as u64) as *mut ProcSched) },
        in_use: 0,
        current: NUM_PROC - 1,
        sched_ptr: 0,
    });
}

#[no_mangle]
pub fn get_curr_cxt_wrap() -> u64 {
    return SCHED.lock().get_curr_cxt();
}

#[no_mangle]
pub fn set_curr_cxt_wrap(rsp:u64) {
    return SCHED.lock().set_curr_cxt(rsp);
}

pub fn _scheduler_init() {
    println!("SCHED");
    SCHED.lock()._clear_sched();
}
