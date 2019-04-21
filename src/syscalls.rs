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
const SYS_wait: usize = 6;

const NUM_SYSCALLS: usize = 7;

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
        self.syscalls.data[SYS_wait] = _sys_wait;
    }

    pub fn _call(&mut self, code:usize) {
        self.syscalls.data[code]();
    }
}

fn _sys_exit() {
    let curr        = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    let status      = curr.cxt.rdi;
    curr.exitstatus = status as u32;
    scheduler::SCHED.lock().bite(curr.spot);
    scheduler::SCHED.lock()._dispatch();
}

fn _sys_fork() {
    let in_use = scheduler::SCHED.lock().get_in_use();
    if in_use >= scheduler::NUM_PROC  {
        //TODO error too many proc
        return;
    }

    let curr     = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    let curr_stk = (curr.stack as *mut stacks::StkBuffer) as u64;
    let stk      = stacks::stk_alloc();
    let pid      = pcbs::PID.lock().get_next_pid();
    let ppid     = curr.pid;
    let children = 0;
    //println!("curr {:p} curr_stk {:x} cxt {:p}", curr, curr_stk, curr.cxt);
    //println!("stk {:x}", stk);

    stacks::stk_copy(curr_stk, stk);

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
        //println!("rsp {:x} curr rsp {:x}, rbp {:x}", cxt_struct.rsp, curr.cxt.rsp, curr.cxt.rbp);
        //cxt_struct.rbp = curr.cxt.rbp - offset_u;
        cxt_struct.rsp = curr.cxt.rsp - offset_u;
    }
    else {
        //cxt_struct.rbp = curr.cxt.rbp + offset_u;
        cxt_struct.rsp = curr.cxt.rbp + offset_u;
    }
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
    cxt_struct.rax  = 0;
    curr.cxt.rax    = pid as u64;
    curr.children  += 1;

    let spot = scheduler::SCHED.lock()._add_proc(cxt, stk, 0, 0, pid, ppid, 0) as i8;
    scheduler::SCHED.lock()._schedule(spot);
}

fn _sys_exec() {
    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    let entry = curr.cxt.rdi;
    let cxt = stacks::_stk_setup(curr.stack, entry);
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
    println!("SYSCALL");
    SYSC.lock()._syscall_init();
    interrupt::INT.lock().__install_isr(INT_VEC_SYSCALL as usize, _sys_isr);
}
