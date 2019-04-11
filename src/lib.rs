#![feature(lang_items)]
#![no_std]
#![feature(const_fn)]

#[macro_use]
extern crate lazy_static;
extern crate spin;

mod c_io;
mod interrupt;

use core::panic::PanicInfo;

#[no_mangle]
pub extern fn rs_sys_init() {
    interrupt::__init_interrupts();
    loop{}
}

#[lang = "eh_personality"]
#[no_mangle]
pub extern fn eh_personality() {}

#[panic_handler]
#[no_mangle]
pub fn panic(_info: &PanicInfo) -> ! {
	loop{}
}
