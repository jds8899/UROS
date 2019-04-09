#![feature(lang_items)]
#![no_std]
#![feature(const_fn)]

mod c_io;

use core::panic::PanicInfo;

#[no_mangle]
pub extern fn rs_sys_init() {
    c_io::print_stuff();
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
