///
/// syscalls.rs
///
/// Author: Jonathan Schenk
///
/// System call implementations
///
////////////////////////////////////////////////////////////////////////////////

use core::ptr;
use core::fmt;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::interrupt;
use crate::scheduler;
use crate::x86arch;
use crate::pcbs;
use crate::clock;
use crate::stacks;
use crate::println;

/// Necessary C/x86 functions
extern "C" {
    #[no_mangle]
    fn _kmalloc(size:u64) -> usize;
    #[no_mangle]
    fn __outb(port:i32, value:i32);
}

/// Syscall codes
const SYS_exit: usize = 0;
const SYS_fork: usize = 1;
const SYS_exec: usize = 2;
const SYS_time: usize = 3;
const SYS_pid:  usize = 4;
const SYS_ppid: usize = 5;
const SYS_wait: usize = 6;

const NUM_SYSCALLS: usize = 7;

static INT_VEC_SYSCALL: i8 = 0x42;

/// Syscall table
struct SysTbl {
    syscalls: &'static mut Buffer,
}

/// Buffer for syscall table
struct Buffer {
    data: [fn(); NUM_SYSCALLS],
}

impl SysTbl {
    /// Initialize syscall table
    pub fn _syscall_init(&mut self) {
        self.syscalls.data[SYS_exit] = _sys_exit;
        self.syscalls.data[SYS_fork] = _sys_fork;
        self.syscalls.data[SYS_exec] = _sys_exec;
        self.syscalls.data[SYS_time] = _sys_time;
        self.syscalls.data[SYS_pid]  = _sys_pid;
        self.syscalls.data[SYS_ppid] = _sys_ppid;
        self.syscalls.data[SYS_wait] = _sys_wait;
    }

    /// Calls a system call
    /// param:
    ///     code: the syscall we want
    pub fn _call(&mut self, code:usize) {
        self.syscalls.data[code]();
    }
}

///
/// _sys_exit - terminates calling process
///
/// implements: sys_exit()
///
/// no return >:)
///
fn _sys_exit() {
    let curr        = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    let status      = curr.cxt.rdi;
    curr.exitstatus = status as u32;
    scheduler::SCHED.lock().bite(curr.spot);
    scheduler::SCHED.lock()._dispatch();
}

///
/// _sys_fork - creates a new process
///
/// implements: sys_fork() -> u16
///
/// returns:
///     parent - PID of new child or 9
///     child  - 0
///
fn _sys_fork() {
    let in_use = scheduler::SCHED.lock().get_in_use();
    if in_use >= scheduler::NUM_PROC  {
        //TODO error too many proc
        return;
    }

    // Get current process info and duplicates it for child
    let curr     = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    let curr_stk = (curr.stack as *mut stacks::StkBuffer) as u64;
    let stk      = stacks::stk_alloc();
    let pid      = pcbs::PID.lock().get_next_pid();
    let ppid     = curr.pid;
    let children = 0;

    stacks::stk_copy(curr_stk, stk);

    // Doing weird stuff to avoid underflow on an unsigned
    let offset     = stk as i64 - curr_stk as i64;
    let offset_u: u64;
    if offset < 0 {
        offset_u = (offset * -1) as u64
    }
    else {
        offset_u = offset as u64;
    }
    let curr_cxt   = (&mut *(curr.cxt) as *mut pcbs::Context) as u64;
    let mut cxt    = curr_cxt;
    if offset < 0 {
        cxt = curr_cxt - offset_u;
    }
    else {
        cxt = curr_cxt + offset_u;
    }
    let cxt_struct = unsafe { &mut *(cxt as *mut pcbs::Context) };
    if offset < 0 {
        //cxt_struct.rbp = curr.cxt.rbp - offset_u;
        cxt_struct.rsp = curr.cxt.rsp - offset_u;
    }
    else {
        //cxt_struct.rbp = curr.cxt.rbp + offset_u;
        cxt_struct.rsp = curr.cxt.rbp + offset_u;
    }

    // This updates the childs base pointers, but apparently Rust doesn't
    // use the bp
/*
    let mut bp      = cxt_struct.rbp as *mut u64;
    let mut bp_data = bp as u64;
    while bp as u64 != 0 && bp_data != 0 {
        unsafe {
            bp_data = ptr::read_volatile(bp);

            if offset < 0 {
                bp_data -= offset_u;
            }
            else {
                bp_data += offset_u;
            }

            ptr::write_volatile(bp, bp_data);
            bp = ptr::read_volatile(bp_data as *mut u64) as *mut u64;
        }
    }
     */

    // Set up returns
    cxt_struct.rax  = 0;
    curr.cxt.rax    = pid as u64;
    curr.children  += 1;

    // Schedule the child
    let spot = scheduler::SCHED.lock()._add_proc(cxt, stk, 0, 0, pid, ppid, 0) as i8;
    scheduler::SCHED.lock()._schedule(spot);
}

///
/// _sys_exec - replace this program with a different one
///
/// implements: sys_exec()
///
/// returns:
///     Doesn't, but probably should on error
///
fn _sys_exec() {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    let entry = curr.cxt.rdi;
    let cxt = stacks::_stk_setup(curr.stack, entry);
    scheduler::set_curr_cxt_wrap(cxt);
}

///
/// _sys_time - get current system time
///
/// implements: sys_time() -> u64
///
/// returns:
///     system time
///
fn _sys_time() {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    curr.cxt.rax = clock::CLK.lock().get_time();
}

///
/// _sys_pid - get the PID of the calling process
///
/// implements: sys_pid() -> u16
///
/// returns:
///     PID of calling process
///
fn _sys_pid() {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    curr.cxt.rax = curr.pid as u64;
}

///
/// _sys_ppid - get the PPID of the calling process
///
/// implements: sys_ppid() -> u16
///
/// returns:
///     PPID of calling process
///
fn _sys_ppid() {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    curr.cxt.rax = curr.ppid as u64;
}

///
/// _sys_wait - wait for child process to terminate
///
/// implements: sys_wait() -> u16
///
/// returns:
///     PID of terminated child or 9 if there ain't one
///
fn _sys_wait() {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };

    if curr.children < 1 {
        curr.cxt.rax = 9 as u64;
        return
    }

    let zombo = scheduler::SCHED.lock().find_zombie(curr.ppid);

    if zombo == 9 {
        //println!("p wait");
        curr.state = pcbs::ST_WAITING;
        scheduler::SCHED.lock()._dispatch();
    }
    else {
        //println!("awaken, father");
        let child = scheduler::SCHED.lock().get_pid(zombo);
        curr.cxt.rax = child as u64;
        scheduler::SCHED.lock().rem_pcb(zombo);
    }

}

///
/// _sys_isr - Get the code for the desired syscall from rax then calls it.
///            Second level call made from call to Systbl.
///
fn _sys_isr(vector:i32, ecode:i32) {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    let mut code = curr.cxt.rax as usize;

    if(code >= NUM_SYSCALLS) {
        code = SYS_exit;
        // TODO Set exit code to bad
    }

    SYSC.lock()._call(code);

    unsafe { __outb(x86arch::PIC_MASTER_CMD_PORT, x86arch::PIC_EOI) };
}

/// Our Global Syscall object
lazy_static! {
    static ref SYSC: Mutex<SysTbl> = Mutex::new(SysTbl {
        syscalls: unsafe { &mut *(_kmalloc(NUM_SYSCALLS as u64 * 8) as *mut Buffer) },
    });
}

///
/// Initialize syscall table
///
pub fn _syscall_init() {
    println!("SYSCALL");
    SYSC.lock()._syscall_init();
    interrupt::INT.lock().__install_isr(INT_VEC_SYSCALL as usize, _sys_isr);
}
