#![feature(lang_items)]
#![no_std]
#![feature(const_fn)]
#![feature(asm)]
#![feature(const_raw_ptr_deref)]

#[macro_use]
extern crate lazy_static;
extern crate spin;

mod c_io;
mod interrupt;
mod x86arch;
mod common;
mod clock;
mod pcbs;
mod scheduler;
mod stacks;
mod users;
mod ulibs;
mod syscalls;

use core::panic::PanicInfo;

extern "C" {
    #[no_mangle]
    fn __isr_restore();
}

#[no_mangle]
pub extern fn rs_sys_init() {
    c_io::WRITER.lock().c_clearscreen();
    c_io::WRITER.lock().c_setscroll(0,7,99,99);
    c_io::WRITER.lock().c_puts_at(0,6,"================================================================================");
    c_io::WRITER.lock().c_puts("System init starting\n");
    c_io::WRITER.lock().c_puts("--------------------\n");
    c_io::WRITER.lock().c_puts("Modules:\n");
    interrupt::__init_interrupts();
    clock::_clk_init();
    stacks::_stk_init();
    scheduler::_scheduler_init();
    syscalls::_syscall_init();
    let entry = (users::init as *mut fn()->i32) as u64;
    let stk_addr = stacks::stk_alloc();
    let stk = unsafe { &mut *(stk_addr as *mut stacks::StkBuffer) };
    println!("stk_addr {:x}", stk_addr);
    let cxt = stacks::_stk_setup(stk, entry);
    scheduler::SCHED.lock()._add_proc(cxt, stk_addr, 0, 0, pcbs::PID_INIT, pcbs::PID_INIT, 0);
    scheduler::SCHED.lock()._schedule(0);
    scheduler::SCHED.lock()._dispatch();
    scheduler::SCHED.lock().dump_curr();
    //loop{}
}

#[lang = "eh_personality"]
#[no_mangle]
pub extern fn eh_personality() {}

#[panic_handler]
#[no_mangle]
pub fn panic(_info: &PanicInfo) -> ! {
    println!{"{}", _info};
    loop{}
}
