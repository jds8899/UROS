use core::ptr;
use core::ffi;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;
use crate::x86arch;
use crate::common;
use crate::c_io;

enum e_states {
    ST_UNUSED,
    ST_NEW,
    ST_RUNNING,
    ST_SLEEPING,
    ST_WAITING,
    ST_BLOCKED_IO,
    ST_KILLED,
    ST_ZOMBIE,
    ST_READY  // must always be last!
}

#[no_mangle]
#[repr(C)]
pub struct Context {
    r15: u64,
    r14: u64,
    r13: u64,
    r12: u64,
    r11: u64,
    r10: u64,
    r9: u64,
    r8: u64,
    rdx: u64,
    rcx: u64,
    rbx: u64,
    rax: u64,
    rdi: u64,
    rsi: u64,
    rbp: u64,
    vector: u64,
    code: u64,
    rip: u64,
    rcs: u64,
    rflags: u64,
    rsp: u64,
    ss: u64,
}

#[no_mangle]
#[repr(C)]
pub struct Pcb {
    cxt: Context,
    //stack: Stack,

    event: u32,
    exitstatus: u32,

    pid: u16,
    ppid: u16,
    children: u16,

    state: e_states,
    ticks: u8,
}
