use core::ptr;
use core::ffi;
use core::mem;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;
use crate::x86arch;
use crate::common;
use crate::pcbs::Pcb;
use crate::pcbs::Context;
use crate::pcbs;

extern "C" {
    #[no_mangle]
    fn _kmalloc(size:u64) -> usize;
    #[no_mangle]
    static do_exit: u64;
}

pub const STACK_SIZE: usize = 1024;

pub struct SysStack {
    sys_stack: &'static mut StkBuffer,
    sys_rsp: u64,
}

pub struct StkBuffer {
    pub data: [u64; STACK_SIZE],
}

impl SysStack {
    pub fn set_rsp(&mut self, val: u64) {
        self.sys_rsp = val;
    }

    pub fn get_rsp(&mut self) -> u64 {
        return self.sys_rsp;
    }

    pub fn get_stack(&mut self) -> u64 {
        let stk = self.sys_stack as *mut StkBuffer;
        return stk as u64;
    }

}

lazy_static! {
    pub static ref STK: Mutex<SysStack> = Mutex::new(SysStack {
        sys_stack: unsafe { &mut *(_kmalloc(STACK_SIZE as u64 * 8) as *mut StkBuffer) },
        sys_rsp: 0,
    });
}

#[no_mangle]
pub fn get_stack_wrap() -> u64 {
    return STK.lock().get_stack();
}

#[no_mangle]
pub fn get_rsp_wrap() -> u64 {
    return STK.lock().get_rsp();
}

pub fn stk_alloc() -> u64 {
    return unsafe { _kmalloc(STACK_SIZE as u64 * 8) as u64};
}

#[no_mangle]
pub fn _stk_setup(s: &'static mut StkBuffer, entry: u64) -> u64 {
    s.data[STACK_SIZE - 1] = 0;
    s.data[STACK_SIZE - 2] = unsafe { do_exit };
    println!("zero {}, exit {:x}, entry {:x}", s.data[STACK_SIZE - 1], s.data[STACK_SIZE - 2], entry);

    let ptr = (&mut s.data[STACK_SIZE - 3] as *mut u64) as u64;
    let ret = ptr - mem::size_of::<pcbs::Context>() as u64;
    let cxt = unsafe { &mut *(ret as *mut Context) };

    cxt.rflags = common::DEFAULT_EFLAGS as u64;
    cxt.rip = entry;
    cxt.rbp = 0;
    cxt.cs = 0x8; // GDT64_CODE
    cxt.ss = 0x10; // GDT64_DATA
    cxt.rsp = ptr;
    return ret;
}

pub fn _stk_init() {
    println!("STK");
    let stk = STK.lock().get_stack();
    let rsp = stk + (STACK_SIZE as u64 * 8) - 2;
    STK.lock().set_rsp(rsp);
}
