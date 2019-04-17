use core::ptr;
use core::ffi;
use core::mem;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;
use crate::x86arch;
use crate::pcbs::Pcb;
use crate::pcbs;

extern "C" {
    #[no_mangle]
    fn _kmalloc(size:u64) -> usize;
    #[no_mangle]
    const do_exit: u64;
}

pub const STACK_SIZE: usize = 1024;

pub struct SysStack {
    sys_stack: &'static mut StkBuffer,
    sys_rsp: u64;
}

pub struct StkBuffer {
    data: [u64; STACK_SIZE],
}

impl SysStack {
    pub fn set_rsp(&mut self, val: u64) {
        self.sys_rsp = val;
    }

    pub fn get_rsp(&mut self) -> u64 {
        return self.sys_rsp;
    }

    pub fn get_stack() -> u64 {
        let stk = self.sys_stack as *mut StkBuffer;
        return stk as u64;
    }
}

lazy_static! {
    pub static ref STK: Mutex<Stack> = Mutex::new(Stack {
        sys_stack: unsafe { &mut *(_kmalloc(STACK_SIZE * 8) as *mut Buffer) },
        sys_rps: 0,
    });
}

pub fn get_stack_wrap() -> u64 {
    return STK.lock().get_stk();
}

pub fn get_rsp_wrap() -> u64 {
    return STK.lock().get_rsp();
}

pub fn _stk_setup(p: &Pcb, entry: u64) {
    p.stk.data[STACK_SIZE - 1] = 0;
    p.stk.data[STACK_SIZE - 2] = do_exit;

    let ptr = (p.stk.data[STACK_SIZE - 3] as *mut u64) as u64;

    p.cxt.rflags = DEFAULT_EFLAGS as u64;
    p.cxt.rip = entry;
    p.cxt.rbp = 0;
    p.cxt.cs = 0x8; // GDT64_CODE
    p.cxt.ss = 0x10; // GDT64_DATA
    p.cxt.rsp = ptr;
}

pub fn _stk_init() {
    println!("STK");
    let stk = STK.lock().get_stk();
    let rsp = stk + (STACK_SIZE * 8) - 2;
    STK.lock().set_rsp(rsp);
}
