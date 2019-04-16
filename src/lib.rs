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
//mod stacks;

use core::panic::PanicInfo;

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
    //loop{}
}

#[lang = "eh_personality"]
#[no_mangle]
pub extern fn eh_personality() {}

#[panic_handler]
#[no_mangle]
pub fn panic(_info: &PanicInfo) -> ! {
	loop{}
}
