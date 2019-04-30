///
/// scheduler.rs
///
/// Author: Jonathan Schenl
///
/// Scheduler implementation
///
////////////////////////////////////////////////////////////////////////////////

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

/// How many processes do we have?
pub const NUM_PROC: u8 = 8;

/// Quantum size
const QUANTUM_STD: u8 = 5;

/// Scheduler struct
pub struct Scheduler {
    proc_stat: &'static mut ProcStatus, // Array telling us if a process is in use
    procs: &'static mut Procs,  // Active queue
    q: &'static mut ProcSched,  // Scheduled process queue
    in_use: u8,    // Number of active boys
    current: u8,   // Current process index
    sched_ptr: u8, // Pointer to where we are in q
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

    ///
    /// Adds a process to the active queue
    ///
    /// param:
    ///     cxt: context pointer as ulong
    ///     stk: pointer to base of the stack as ulong
    ///     event: not really used by anything
    ///     extst: exit status, shouldn't really be set here
    ///     pid: process id
    ///     ppid: parent process id
    ///     children: number of children (always 0 here)
    ///
    /// returns:
    ///     index into active queue of new process
    ///
    pub fn _add_proc(&mut self, cxt: u64, stk:u64, event:u32, extst:u32,
                     pid:u16, ppid:u16, children:u16) -> usize {
        if self.in_use == 7 {
            return 9;
        }
        let mut next = 0 as usize;
        // First process
        if self.in_use == 0 {
            next = 0;

            self.procs.data[0].spot = 0;
            self.proc_stat.data[0]  = 1;
        }
        // Useless block, whoops
        else if self.in_use == NUM_PROC {
            //None
            return 9;
        }
        // Find empty process in active queue
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

        // Do our gross casts
        unsafe {
            self.procs.data[next].cxt        = &mut *(cxt as *mut pcbs::Context);

            self.procs.data[next].stack      = &mut *(stk as *mut stacks::StkBuffer);
        }

        // Set up the rest
        self.procs.data[next].event      = event;
        self.procs.data[next].exitstatus = extst;

        self.procs.data[next].pid        = pid;
        self.procs.data[next].ppid       = ppid;
        self.procs.data[next].children   = children;
        self.procs.data[next].state      = pcbs::ST_READY;

        return next;
        //self.procs.data[next].state = pcbs::e_states::ST_READY;
    }

    ///
    /// Schedule a process to run as long as it is ready
    ///
    /// param:
    ///     ind: index of process in active
    ///
    pub fn _schedule(&mut self, ind:i8) {
        if self.q.data[self.sched_ptr as usize] == -1 &&
            self.procs.data[ind as usize].state != pcbs::ST_WAITING &&
            self.procs.data[ind as usize].state != pcbs::ST_ZOMBIE {

            self.q.data[self.sched_ptr as usize] = ind;
            self.sched_ptr = (self.sched_ptr + 1) % NUM_PROC;
        }
    }

    ///
    /// Give the scheduled process the CPU
    ///
    pub fn _dispatch(&mut self) {
        self.q.data[self.current as usize] = -1;
        self.current = (self.current + 1) % NUM_PROC;

        let ind  = self.q.data[self.current as usize] as usize;
        self.procs.data[ind].state = pcbs::ST_RUNNING;
        self.procs.data[ind].ticks = QUANTUM_STD;
    }

    ///
    /// Gets pointer to curr proc's context
    ///
    /// returns:
    ///     ulong that points to context
    ///
    pub fn get_curr_cxt(&mut self) -> u64 {
        let ind  = self.q.data[self.current as usize] as usize;
        let curr = &mut *(self.procs.data[ind].cxt) as *mut pcbs::Context;
        return curr as u64;
    }

    ///
    /// Gets pointer to curr proc
    ///
    /// returns:
    ///     ulong that points to the process struct
    ///
    pub fn get_curr(&mut self) -> u64 {
        let ind  = self.q.data[self.current as usize] as usize;
        let curr = &mut (self.procs.data[ind]) as *mut Pcb;
        return curr as u64;
    }

    ///
    /// Sets pointer to curr proc's context
    ///
    /// param:
    ///     rsp: ulong that points to context
    ///
    pub fn set_curr_cxt(&mut self, rsp:u64) {
        let ind  = self.q.data[self.current as usize] as usize;
        self.procs.data[ind].cxt = unsafe { &mut *(rsp as *mut pcbs::Context) };
    }

    ///
    /// Dump some of the info about the current process
    ///
    pub fn dump_curr(&mut self) {
        let curr = &self.procs.data[self.current as usize];
        println!("cxt: {:p}",curr.cxt);
        println!("rsp: {:x}",curr.cxt.rsp);
        println!("stk: {:p}",curr.stack);
        println!("pid: {:x}",curr.pid);
        println!("ppid: {:x}",curr.ppid);
        println!("children: {:x}",curr.children);
    }

    ///
    /// Clears removes all processes from the process queue
    ///
    pub fn _clear_sched(&mut self) {
        for i in 0..NUM_PROC {
            self.q.data[i as usize] = -1;
        }
    }

    ///
    /// Tells caller how many processes are in the active queue
    ///
    pub fn get_in_use(&mut self) -> u8 {
        return self.in_use;
    }

    ///
    /// Turns process into a zombie
    ///
    /// param:
    ///     ind: index of process to bite in active queue
    ///
    pub fn bite(&mut self, ind: i8) {

        // Reparent the zombie's children init
        if self.procs.data[ind as usize].children > 0 {
            for i in 0..NUM_PROC {
                if i as i8 != ind {
                    if self.procs.data[ind as usize].ppid == ind as u16 {
                        self.procs.data[i as usize].ppid        = 0;
                        self.procs.data[ind as usize].children -= 1;
                        self.procs.data[0].children            += 1;
                    }
                }
            }
        }

        // Find the index of zombie's parent
        let mut parent = 0 as usize;
        for i in 0..NUM_PROC {
            if self.procs.data[i as usize].pid == self.procs.data[ind as usize].ppid {
                parent = i as usize;
            }
        }
        // If parent is waiting, cleanup zombie
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
        // If not, issa zombie
        else {
            //println!("p not found");
            self.procs.data[ind as usize].state = pcbs::ST_ZOMBIE;
        }

    }

    ///
    /// Finds the zombie child of a waiting parent
    ///
    /// param:
    ///     ppid: parent that is waiting
    ///
    /// returns:
    ///     9 if no zombie child, child's index in active queue if it exists
    ///
    pub fn find_zombie(&mut self, ppid: u16) -> i8 {
        let mut ret = 9 as i8;
        for i in 0..NUM_PROC {
            if self.procs.data[i as usize].ppid == ppid &&
                self.procs.data[i as usize].state == pcbs::ST_ZOMBIE {
                    ret = i as i8;
                    break;
            }
        }
        return ret
    }

    ///
    /// Gets pid of process at given index in active queue
    ///
    /// param:
    ///     ind: index in active queue
    ///
    /// returns:
    ///     pid of process at ind
    ///
    pub fn get_pid(&mut self, ind:i8) -> u16 {
        return self.procs.data[ind as usize].pid;
    }

    ///
    /// Cleans up a process (should free stack, though)
    ///
    /// param:
    ///     ind: index of process to clean up
    ///
    pub fn rem_pcb(&mut self, ind:i8) {
        self.proc_stat.data[ind as usize] = 0;
        self.procs.data[ind as usize].state = pcbs::ST_UNUSED;
        self.in_use -= 1;
    }
}

/// Scheduler global
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

/// Wraps call to get_curr_cxt for external assembly use
#[no_mangle]
pub fn get_curr_cxt_wrap() -> u64 {
    return SCHED.lock().get_curr_cxt();
}

/// Wraps call to set_curr_cxt for external assembly use
#[no_mangle]
pub fn set_curr_cxt_wrap(rsp:u64) {
    return SCHED.lock().set_curr_cxt(rsp);
}

/// Initializes the scheduler
pub fn _scheduler_init() {
    println!("SCHED");
    SCHED.lock()._clear_sched();
}
