use core::ptr;
use core::ffi;
use core::mem;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;
use crate::x86arch;
use crate::pcbs::Pcb;

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
    data: [u8; NUM_PROC],
}

impl Scheduler {
    pub fn _schedule(&mut self, pcb: &Pcb) {
        let mut next = 0 as usize;
        if self.in_use == 0 {
            next = 0;
            self.proc_stat[0] = 1;
        }
        else if self.in_use == NUM_PROC {
            //None
            return;
        }
        else {
            for i in 1..NUM_PROC {
                let next_ind = (i + self.curr) % NUM_PROC;
                if self.proc_stat[next_ind] == 0 {
                    next = next_ind;
                    self.proc_stat[next_ind] = 1;
                }
            }
        }
        self.in_use += 1;

        // Misery
        self.procs[next].context.r15 = pcb.context.r15;
        self.procs[next].context.r14 = pcb.context.r14;
        self.procs[next].context.r13 = pcb.context.r13;
        self.procs[next].context.r12 = pcb.context.r12;
        self.procs[next].context.r11 = pcb.context.r11;
        self.procs[next].context.r10 = pcb.context.r10;
        self.procs[next].context.r9 = pcb.context.r9;
        self.procs[next].context.r8 = pcb.context.r8;
        self.procs[next].context.rdx = pcb.context.rdx;
        self.procs[next].context.rcx = pcb.context.rcx;
        self.procs[next].context.rbx = pcb.context.rbx;
        self.procs[next].context.rax = pcb.context.rax;
        self.procs[next].context.rdi = pcb.context.rdi;
        self.procs[next].context.rsi = pcb.context.rsi;
        self.procs[next].context.rbp = pcb.context.rbp;
        self.procs[next].context.vector = pcb.context.vector;
        self.procs[next].context.code = pcb.context.code;
        self.procs[next].context.rip = pcb.context.rip;
        self.procs[next].context.cs = pcb.context.cs;
        self.procs[next].context.rflags = pcb.context.rflags;
        self.procs[next].context.rsp = pcb.context.rsp;
        self.procs[next].context.ss = pcb.context.ss;
        //TODO stack
        self.procs[next].event = pcb.event;
        self.procs[next].exitstatus = pcb.exitstatus;

        self.procs[next].pid = pcb.pid;
        self.procs[next].ppid = pcb.ppid;
        self.procs[next].children = pcb.children;

        self.procs[next].state = pcbs::e_states.ST_READY;
    }

    pub fn dispatch() {
        for i in 1..NUM_PROC {
            let next_ind = (i + self.curr) % NUM_PROC;
            if self.proc_stat[next_ind] == 1 && next_ind != self.curr {
                self.curr = next_ind;
                break;
            }
        }

        self.procs[curr].state = pcbs::e_states.ST_RUNNING;
        self.procs[curr].ticks = QUANTUM_STD;
    }
}

lazy_static! {
    pub static ref INT: Mutex<Interrupt> = Mutex::new(Interrupt {
        proc_stat: unsafe { &mut *(_kmalloc(NUM_PROCS) as *mut Buffer) },
        procs: unsafe { &mut *(_kmalloc(NUM_PROCS * (mem::size_of(Pcb) as u64)) as *mut Buffer) },
        in_use: 0,
        current: 0,
    });
}

struct Procs {
    data: [mem::size_of(Pcb); NUM_PROC],
}
