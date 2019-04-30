///
/// stacks.rs
///
/// Author: Jonathan Schenk
///
/// This file contains code for setting up user stacks and holds the OS'
/// system stack.
///
////////////////////////////////////////////////////////////////////////////////

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

/// We should probably have kfree in here too, oops
extern "C" {
    #[no_mangle]
    fn _kmalloc(size:u64) -> usize;
    #[no_mangle]
    fn do_exit();
}

pub const STACK_SIZE: usize = 1024;

/// The system's kernel stack
pub struct SysStack {
    sys_stack: &'static mut StkBuffer,
    sys_rsp: u64,
}

/// Stack type
pub struct StkBuffer {
    pub data: [u64; STACK_SIZE],
}

impl SysStack {
    ///
    /// set_rsp - sets the system's stack pointer
    ///
    /// param:
    ///     val: value to set rsp to
    ///
    pub fn set_rsp(&mut self, val: u64) {
        self.sys_rsp = val;
    }

    ///
    /// get_rsp - gets the system's stack pointer
    ///
    /// return:
    ///     kernel rsp
    ///
    pub fn get_rsp(&mut self) -> u64 {
        return self.sys_rsp;
    }

    ///
    /// get_stack - gets the system's stack
    ///
    /// return:
    ///     A 64 bit address that points to the base of the kernel stack
    ///
    pub fn get_stack(&mut self) -> u64 {
        let stk = self.sys_stack as *mut StkBuffer;
        return stk as u64;
    }

}

/// Our system stack global
lazy_static! {
    pub static ref STK: Mutex<SysStack> = Mutex::new(SysStack {
        sys_stack: unsafe { &mut *(_kmalloc(STACK_SIZE as u64 * 8) as *mut StkBuffer) },
        sys_rsp: 0,
    });
}

/// Wraps call to get_stack for external assembly code
#[no_mangle]
pub fn get_stack_wrap() -> u64 {
    return STK.lock().get_stack();
}

/// Wraps call to get_rsp for external assembly code
#[no_mangle]
pub fn get_rsp_wrap() -> u64 {
    return STK.lock().get_rsp();
}

///
/// stk_alloc - allocates a stack for caller
///
/// returns:
///     An 64 bit that points to the base of the stack
///
pub fn stk_alloc() -> u64 {
    return unsafe { _kmalloc(STACK_SIZE as u64 * 8) as u64};
}

///
/// stk_copy - copies the contents of one stack into another
///
/// params:
///     src: source stack
///     dst: destination stack
///
pub fn stk_copy(src:u64, dst:u64) {
    let src_buff = unsafe { &mut *(src as *mut StkBuffer) };
    let dst_buff = unsafe { &mut *(dst as *mut StkBuffer) };
    for i in 0..STACK_SIZE {
        dst_buff.data[i] = src_buff.data[i];
    }
}

///
/// _stk_setup - sets up the stack for a new process
///
/// params:
///     s: process stack
///     entry: entry point for process
///
/// returns:
///     64 bit address of the base of the context block for this stack
///
#[no_mangle]
pub fn _stk_setup(s: &'static mut StkBuffer, entry: u64) -> u64 {
    // Get address of _sys_exit
    let ext = (do_exit as *mut fn()) as u64;

    // Put 0 at last index, and exit as return address
    s.data[STACK_SIZE - 1] = 0;
    s.data[STACK_SIZE - 2] = ext;

    // Set up context block
    let ptr = (&mut s.data[STACK_SIZE - 2] as *mut u64) as u64;
    let ret = ptr - mem::size_of::<pcbs::Context>() as u64;
    let cxt = unsafe { &mut *(ret as *mut Context) };

    // Set up registers for the process
    cxt.rflags = common::DEFAULT_EFLAGS as u64;
    cxt.rip = entry;
    cxt.rbp = 0;
    cxt.cs = 0x8; // GDT64_CODE
    cxt.ss = 0x10; // GDT64_DATA
    cxt.rsp = ptr;
    return ret;
}

///
/// Initializes system stack
///
pub fn _stk_init() {
    println!("STK");
    let stk = STK.lock().get_stack();
    let rsp = stk + (STACK_SIZE as u64 * 8) - 2;
    STK.lock().set_rsp(rsp);
}
