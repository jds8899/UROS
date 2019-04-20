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

fn _sys_fork() {
    let in_use = scheduler::SCHED.lock().get_in_use();
    if in_use >= NUM_PROC  {
        //TODO error too many proc
        return;
    }

    let curr     = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    let curr_stk = (curr.stk as *mut StkBuffer) as u64;
    let stk      = stacks::stk_alloc();
    let pid      = pcbs::PID.lock().get_next_pid();
    let ppid     = curr.pid;
    let children = 0;

    stacks::stk_copy(curr_stk, stk);

    let offset     = stk - curr_stk;
    let curr_cxt   = (&mut *(curr.cxt) as *mut pcbs::Context) as u64;
    let cxt        = curr_cxt + offset;
    let cxt_struct = unsafe { &mut *(cxt as *mut pcbs::Context) };
    cxt_struct.rbp += offset;
    cxt_struct.rsp += offset;

    let bp      = cxt_struct.rbp as *mut u64;
    let bp_data = bp;
    while bp && bp_data {
        bp_data = ptr::read_volatile(bp);
        bp_data += offset;
        ptr::write_volatile(bp, bp_data);
        bp = ptr::read_volatile(bp_data as *mut u64) as *mut u64;
    }

    cxt_struct.rax += 0;
    curr.cxt.rax    = pid;
    curr.children  += 1;

    let spot = scheduler::SCHED.lock()._add_proc(cxt, stk, 0, 0, pid, ppid, 0) as i8;
    scheduler::SCHED.lock()._schedule(spot);
}

fn _sys_exec() {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    let entry = curr.cxt.rdi;
    let cxt = stacks::_stk_setup(curr.stk, entry);
    scheduler::set_curr_cxt_wrap(cxt);
}

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
