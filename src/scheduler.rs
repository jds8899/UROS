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
                let next_ind = ((i + self.current) % NUM_PROC) as usize;
                if self.proc_stat.data[next_ind] == 0 {
                    next = next_ind;
                    self.procs.data[next_ind].spot = next_ind as i8;
                    self.proc_stat.data[next_ind]  = 1;
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

        return next;
        //self.procs.data[next].state = pcbs::e_states::ST_READY;
    }

    pub fn _schedule(&mut self, ind:i8) {
        if self.q.data[self.sched_ptr as usize] == -1 {
            self.q.data[self.sched_ptr as usize] = ind;
            self.sched_ptr = (self.sched_ptr + 1) % NUM_PROC;
        }
    }

    pub fn _dispatch(&mut self) {
        self.q.data[self.current as usize] = -1;
        self.current = (self.current + 1) % NUM_PROC;

        let ind  = self.q.data[self.current as usize] as usize;
        self.procs.data[ind].state = pcbs::e_states::ST_RUNNING;
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
    SCHED.lock()._clear_sched();
}
