pub static EFLAGS_MB1: i32 = 0x00000002;
pub static EFLAGS_IF: i32 = 0x00000200;

pub static TIMER_BASE_PORT: i32 = 0x40;
pub static TIMER_0_PORT: i32 = (TIMER_BASE_PORT);
pub static TIMER_1_PORT: i32 = (TIMER_BASE_PORT + 1);
pub static TIMER_2_PORT: i32 = (TIMER_BASE_PORT + 2);
pub static TIMER_CONTROL_PORT: i32 = (TIMER_BASE_PORT + 3);

pub static TIMER_MODE_0: i32 = 0x00;
pub static TIMER_MODE_1: i32 = 0x02;
pub static TIMER_MODE_2: i32 = 0x04;
pub static TIMER_MODE_3: i32 = 0x06;
pub static TIMER_MODE_4: i32 = 0x08;
pub static TIMER_MODE_5: i32 = 0x0a;

pub static TIMER_0_LOAD: i32 = 0x30;
pub static TIMER_0_SQUARE: i32 = TIMER_MODE_3;

pub static INT_VEC_KEYBOARD: usize = 0x21;
pub static INT_VEC_TIMER: usize = 0x20;
pub static INT_VEC_MYSTERY: usize = 0x27;

pub static IDT_PRESENT: usize = 0x8000;
pub static IDT_DPL_0: usize = 0x0000;
pub static IDT_INT32_GATE: usize = 0x0e00;

pub static PIC_NEEDICW4: i32 = 0x01;
pub static PIC_ICW1BASE: i32 = 0x10;
pub static PIC_86MODE: i32 = 0x01;
pub static PIC_EOI: i32 = 0x20;

pub static PIC_MASTER_CMD_PORT: i32 = 0x20;
pub static PIC_MASTER_IMR_PORT: i32 = 0x21;
pub static PIC_SLAVE_CMD_PORT: i32 = 0xA0;
pub static PIC_SLAVE_IMR_PORT: i32 = 0xA1;
pub static PIC_MASTER_SLAVE_LINE: i32 = 0x04;
pub static PIC_SLAVE_ID: i32 = 0x02;
