use core::ptr;
use core::ffi;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;
use crate::x86arch;

extern "C" {
    #[no_mangle]
    fn __outb(port:i32, value:i32);
    #[no_mangle]
    static __isr_stub_table: usize;
}

pub struct Clock {
    _pinwheel: i32,
    _pindex: u32,
    _system_time: u64,
}
