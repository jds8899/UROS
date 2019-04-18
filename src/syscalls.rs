use core::ptr;
use core::fmt;
use spin::Mutex;
use lazy_static::lazy_static;

extern "C" {
    #[no_mangle]
    fn _kmalloc(size:u64) -> usize;
}

const NUM_SYSCALLS: usize = 6;

struct SysTbl {
    syscalls: &'static mut Buffer,
}

struct Buffer {
    data: [fn(); NUM_SYSCALL],
}

lazy_static! {
    pub static ref INT: Mutex<Interrupt> = Mutex::new(Interrupt {
        syscalls: unsafe { &mut *(_kmalloc(NUM_SYSCALL * 8) as *mut Buffer) },
    });
}
