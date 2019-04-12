
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
