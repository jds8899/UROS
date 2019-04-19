use core::ptr;
use core::fmt;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::interrupt;
use crate::scheduler;
use crate::x86arch;
use crate::pcbs;
use crate::clock;
use crate::println;

extern "C" {
    #[no_mangle]
    fn _kmalloc(size:u64) -> usize;
    #[no_mangle]
    fn __outb(port:i32, value:i32);
}

const SYS_exit: usize = 0;
const SYS_fork: usize = 1;
const SYS_exec: usize = 2;
const SYS_time: usize = 3;
const SYS_pid:  usize = 4;
const SYS_ppid: usize = 5;

const NUM_SYSCALLS: usize = 6;

static INT_VEC_SYSCALL: i8 = 0x42;

struct SysTbl {
    syscalls: &'static mut Buffer,
}

struct Buffer {
    data: [fn(); NUM_SYSCALLS],
}

impl SysTbl {
    pub fn _syscall_init(&mut self) {
        self.syscalls.data[SYS_exit] = _sys_exit;
        self.syscalls.data[SYS_fork] = _sys_fork;
        self.syscalls.data[SYS_exec] = _sys_exec;
        self.syscalls.data[SYS_time] = _sys_time;
        self.syscalls.data[SYS_pid]  = _sys_pid;
        self.syscalls.data[SYS_ppid] = _sys_ppid;
    }

    pub fn _call(&mut self, code:usize) {
        self.syscalls.data[code]();
    }
}

fn _sys_exit() {}

fn _sys_fork() {}

fn _sys_exec() {}

fn _sys_time() {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    curr.cxt.rax = clock::CLK.lock().get_time();
}

fn _sys_pid() {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    curr.cxt.rax = curr.pid as u64;
}

fn _sys_ppid() {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    curr.cxt.rax = curr.ppid as u64;
}

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

lazy_static! {
    static ref SYSC: Mutex<SysTbl> = Mutex::new(SysTbl {
        syscalls: unsafe { &mut *(_kmalloc(NUM_SYSCALLS as u64 * 8) as *mut Buffer) },
    });
}

pub fn _syscall_init() {
    SYSC.lock()._syscall_init();
    interrupt::INT.lock().__install_isr(INT_VEC_SYSCALL as usize, _sys_isr);
}
