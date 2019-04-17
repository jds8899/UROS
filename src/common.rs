use crate::x86arch;

pub static CLOCK_FREQUENCY: i32 = 1000;
pub static TIMER_FREQUENCY: i32 = 1193182;
pub static DEFAULT_EFLAGS: i32 = (x86arch::EFLAGS_MB1 | x86arch::EFLAGS_IF);
