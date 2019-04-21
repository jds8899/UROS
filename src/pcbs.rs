use core::ptr;
use core::ffi;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;
use crate::x86arch;
use crate::common;
use crate::c_io;
use crate::stacks::StkBuffer;

pub static ST_UNUSED: u8 = 0;
pub static ST_NEW: u8 = 1;
pub static ST_RUNNING: u8 = 2;
pub static ST_SLEEPING: u8 = 3;
pub static ST_WAITING: u8 = 4;
pub static ST_BLOCKED_IO: u8 = 5;
pub static ST_KILLED: u8 = 6;
pub static ST_ZOMBIE: u8 = 7;
pub static ST_READY: u8 = 8;  // must always be last!

pub const PID_INIT: u16 = 1;
const PID_FIRST: u16 = 100;

pub struct Pids {
    curr: u16,
}

impl Pids {
    pub fn get_next_pid(&mut self) -> u16 {
        let ret = self.curr;
        self.curr += 1;
        return ret;
    }
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
    pub cxt: &'static mut Context,
    pub stack: &'static mut StkBuffer,

    pub event: u32,
    pub exitstatus: u32,

    pub pid: u16,
    pub ppid: u16,
    pub children: u16,

    pub state: u8,
    pub ticks: u8,
    pub spot: i8,
}

lazy_static! {
    pub static ref PID: Mutex<Pids> = Mutex::new(Pids {
        curr: PID_FIRST,
    });
}
