use core::ptr;
use core::ffi;
use core::mem;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;
use crate::x86arch;
use crate::pcbs::Pcb;
use crate::pcbs;

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
    pub fn _schedule(&mut self, pcb: &Pcb) {
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
        //TODO stack
        self.procs.data[next].event      = pcb.event;
        self.procs.data[next].exitstatus = pcb.exitstatus;

        self.procs.data[next].pid        = pcb.pid;
        self.procs.data[next].ppid       = pcb.ppid;
        self.procs.data[next].children   = pcb.children;

        self.procs.data[next].state = pcbs::e_states::ST_READY;
    }

    pub fn dispatch(&mut self) {
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
}

lazy_static! {
    pub static ref SCHED: Mutex<Scheduler> = Mutex::new(Scheduler {
        proc_stat: unsafe { &mut *(_kmalloc(NUM_PROC as u64) as *mut ProcStatus) },
        procs: unsafe { &mut *(_kmalloc((NUM_PROC as u64) * (mem::size_of::<Pcb>() as u64)) as *mut Procs) },
        in_use: 0,
        current: 0,
    });
}
