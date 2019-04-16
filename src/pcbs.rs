use core::ptr;
use core::ffi;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;
use crate::x86arch;
use crate::common;
use crate::c_io;

pub enum e_states {
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
    pub r15: u64,
    pub r14: u64,
    pub r13: u64,
    pub r12: u64,
    pub r11: u64,
    pub r10: u64,
    pub r9: u64,
    pub r8: u64,
    pub rdx: u64,
    pub rcx: u64,
    pub rbx: u64,
    pub rax: u64,
    pub rdi: u64,
    pub rsi: u64,
    pub rbp: u64,
    pub vector: u64,
    pub code: u64,
    pub rip: u64,
    pub cs: u64,
    pub rflags: u64,
    pub rsp: u64,
    pub ss: u64,
}

#[no_mangle]
#[repr(C)]
pub struct Pcb {
    pub cxt: Context,
    //stack: Stack,

    pub event: u32,
    pub exitstatus: u32,

    pub pid: u16,
    pub ppid: u16,
    pub children: u16,

    pub state: e_states,
    pub ticks: u8,
}
