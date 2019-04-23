use core::ptr;
use core::ffi;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;
use crate::x86arch;
use crate::common;
use crate::interrupt;
use crate::scheduler;
use crate::pcbs;
use crate::stacks;
use crate::c_io;

extern "C" {
    #[no_mangle]
    fn __outb(port:i32, value:i32);
    #[no_mangle]
    static __isr_stub_table: usize;
}

static pin: [char; 4] = ['|', '/', '-', '\\'];

pub struct Clock {
    pinwheel: i32,
    pindex: u32,
    system_time: u64,
}

impl Clock {
    pub fn set_time(&mut self, t:u64) {
        self.system_time = t;
    }

    pub fn incr_time(&mut self) {
        self.system_time += 1;
    }

    pub fn get_time(&mut self) -> u64 {
        return self.system_time;
    }

    pub fn pin_deal(&mut self) {
        self.pinwheel += 1;
        if(self.pinwheel == (common::CLOCK_FREQUENCY / 10)) {
            self.pinwheel = 0;
            self.pindex += 1;
            let ind = (self.pindex & 3) as usize;
            c_io::WRITER.lock().c_putchar_at(79, 0, pin[ind] as u8);
        }
    }
}

pub fn _clk_isr(vector:i32, code:i32) {
    CLK.lock().pin_deal();
    CLK.lock().incr_time();

    let curr = unsafe { &mut *(scheduler::SCHED.lock().get_curr() as *mut pcbs::Pcb) };
    let cxt = (curr.cxt as *mut pcbs::Context) as u64;
    let stk = (curr.stack as *mut stacks::StkBuffer) as u64;
    curr.ticks -= 1;
    if curr.ticks < 1 {
        scheduler::SCHED.lock()._schedule(curr.spot);
        scheduler::SCHED.lock()._dispatch();
    }

    //println!("{:X}",CLK.lock().get_time());
    unsafe { __outb(x86arch::PIC_MASTER_CMD_PORT, x86arch::PIC_EOI) };
}

lazy_static! {
    pub static ref CLK: Mutex<Clock> = Mutex::new(Clock {
        pinwheel: (common::CLOCK_FREQUENCY / 10) - 1,
        pindex: 0,
        system_time: 0,
    });
}

pub fn _clk_init() {
    let mut divisor: i32;
    println!("CLOCK");

    divisor = common::TIMER_FREQUENCY / common::CLOCK_FREQUENCY;
    unsafe {
        __outb(x86arch::TIMER_CONTROL_PORT, x86arch::TIMER_0_LOAD | x86arch::TIMER_0_SQUARE);
        __outb(x86arch::TIMER_0_PORT, divisor & 0xff);
        __outb(x86arch::TIMER_0_PORT, (divisor >> 8) & 0xff);
    }

    interrupt::INT.lock().__install_isr(x86arch::INT_VEC_TIMER, _clk_isr);
}
