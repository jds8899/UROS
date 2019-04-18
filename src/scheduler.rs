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

const NUM_PROC: u8 = 8;
const QUANTUM_STD: u8 = 5;

pub struct Scheduler {
    proc_stat: &'static mut ProcStatus,
    procs: &'static mut Procs,
    in_use: u8,
    current: u8,
}

struct ProcStatus {
    data: [u8; (NUM_PROC as usize)],
}

struct Procs {
    data: [Pcb; (NUM_PROC as usize)],
}

impl Scheduler {
    pub fn _schedule(&mut self, cxt: u64, stk:u64, event:u32, extst:u32,
                     pid:u16, ppid:u16, children:u16) {
        let mut next = 0 as usize;
        if self.in_use == 0 {
            next = 0;
            self.proc_stat.data[0] = 1;
        }
        else if self.in_use == NUM_PROC {
            //None
            return;
        }
        else {
            for i in 1..NUM_PROC {
                let next_ind = ((i + self.current) % NUM_PROC) as usize;
                if self.proc_stat.data[next_ind] == 0 {
                    next = next_ind;
                    self.proc_stat.data[next_ind] = 1;
                }
            }
        }
        self.in_use += 1;

        /*
        // Misery
        self.procs.data[next].cxt.r15    = pcb.cxt.r15;
        self.procs.data[next].cxt.r14    = pcb.cxt.r14;
        self.procs.data[next].cxt.r13    = pcb.cxt.r13;
        self.procs.data[next].cxt.r12    = pcb.cxt.r12;
        self.procs.data[next].cxt.r11    = pcb.cxt.r11;
        self.procs.data[next].cxt.r10    = pcb.cxt.r10;
        self.procs.data[next].cxt.r9     = pcb.cxt.r9;
        self.procs.data[next].cxt.r8     = pcb.cxt.r8;
        self.procs.data[next].cxt.rdx    = pcb.cxt.rdx;
        self.procs.data[next].cxt.rcx    = pcb.cxt.rcx;
        self.procs.data[next].cxt.rbx    = pcb.cxt.rbx;
        self.procs.data[next].cxt.rax    = pcb.cxt.rax;
        self.procs.data[next].cxt.rdi    = pcb.cxt.rdi;
        self.procs.data[next].cxt.rsi    = pcb.cxt.rsi;
        self.procs.data[next].cxt.rbp    = pcb.cxt.rbp;
        self.procs.data[next].cxt.vector = pcb.cxt.vector;
        self.procs.data[next].cxt.code   = pcb.cxt.code;
        self.procs.data[next].cxt.rip    = pcb.cxt.rip;
        self.procs.data[next].cxt.cs     = pcb.cxt.cs;
        self.procs.data[next].cxt.rflags = pcb.cxt.rflags;
        self.procs.data[next].cxt.rsp    = pcb.cxt.rsp;
        self.procs.data[next].cxt.ss     = pcb.cxt.ss;
        */

        unsafe {
            self.procs.data[next].cxt        = &mut *(cxt as *mut pcbs::Context);

            self.procs.data[next].stack      = &mut *(stk as *mut stacks::StkBuffer);
        }

        self.procs.data[next].event      = event;
        self.procs.data[next].exitstatus = extst;

        self.procs.data[next].pid        = pid;
        self.procs.data[next].ppid       = ppid;
        self.procs.data[next].children   = children;

        self.procs.data[next].state = pcbs::e_states::ST_READY;
    }

    pub fn dispatch(&mut self) {
        self.proc_stat.data[self.current as usize] = 0;
        self.in_use -= 1;
        for i in 1..NUM_PROC {
            let next_ind = ((i + self.current) % NUM_PROC) as usize;
            if self.proc_stat.data[next_ind] == 1 && next_ind != self.current as usize {
                self.current = next_ind as u8;
                break;
            }
        }

        self.procs.data[self.current as usize].state = pcbs::e_states::ST_RUNNING;
        self.procs.data[self.current as usize].ticks = QUANTUM_STD;
    }

    pub fn get_curr_cxt(&mut self) -> u64 {
        let curr = &mut *(self.procs.data[self.current as usize].cxt) as *mut pcbs::Context;
        return curr as u64;
    }

    pub fn get_curr(&mut self) -> u64 {
        let curr = &mut (self.procs.data[self.current as usize]) as *mut Pcb;
        return curr as u64;
    }

    pub fn set_curr_cxt(&mut self, rsp:u64) {
        self.procs.data[self.current as usize].cxt = unsafe { &mut *(rsp as *mut pcbs::Context) };
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
}

lazy_static! {
    pub static ref SCHED: Mutex<Scheduler> = Mutex::new(Scheduler {
        proc_stat: unsafe { &mut *(_kmalloc(NUM_PROC as u64) as *mut ProcStatus) },
        procs: unsafe { &mut *(_kmalloc((NUM_PROC as u64) * (mem::size_of::<Pcb>() as u64)) as *mut Procs) },
        in_use: 0,
        current: 0,
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
