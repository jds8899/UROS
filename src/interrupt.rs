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
    fn _kmalloc(size:u64) -> usize;
    #[no_mangle]
    static __isr_stub_table: usize;
}

const ISR_TAB_USIZE: usize = 256;
const ISR_TAB_SIZE: u64 = 256;
static IDT_ADDRESS: usize = 0x00001100;

pub struct Interrupt {
    isr_table: &'static mut Buffer,
}

#[no_mangle]
#[repr(C)]
struct idt_gate {
    offset_15_0: u16,
    segment_selector: u16,
    flags: u16,
    offset_31_16: u16,
    offset_63_32: u32,
    zero: u32
}

struct Buffer {
    data: [fn(i32, i32); ISR_TAB_USIZE],
}

impl Interrupt {
    pub fn init_idt(&mut self) {
        let mut idt_addr = unsafe { __isr_stub_table };
        for i in 0..ISR_TAB_USIZE {
            self.set_idt_entry(i, idt_addr);
            self.__install_isr(i, __default_unexpected_handler);
            if i == 0x08 || (i >= 10 && i <= 14) || i == 17 {
                idt_addr += 7;
            }
            else {
                idt_addr += 9;
            }
        }

        self.__install_isr(x86arch::INT_VEC_KEYBOARD, __default_expected_handler);
        self.__install_isr(x86arch::INT_VEC_TIMER, __default_expected_handler);
        self.__install_isr(x86arch::INT_VEC_MYSTERY, __default_mystery_handler);
    }

    pub fn set_idt_entry(&mut self, entry:usize, handler:usize) {
        let addr = (IDT_ADDRESS + entry * 16) as usize;
        let g = unsafe { &mut *((addr) as *mut idt_gate) };

        g.offset_15_0 = handler as u16 & 0xffff;
        g.segment_selector = 0x0008;
        g.flags = x86arch::IDT_PRESENT as u16 |
        x86arch::IDT_DPL_0 as u16 | x86arch::IDT_INT32_GATE as u16;
        g.offset_31_16 = (handler >> 16) as u16 & 0xffff as u16;
        g.offset_63_32 = (handler >> 32) as u32 & 0xffffffff as u32;
        g.zero = 0x00000000;
    }

    pub fn __install_isr(&mut self, vector:usize, handler:fn(i32, i32)) -> fn(i32, i32) {
        let old_handler = self.isr_table.data[vector];
        self.isr_table.data[vector] = handler;
        return old_handler;
    }

    pub fn init_pic(&mut self) {
        unsafe {
            __outb(x86arch::PIC_MASTER_CMD_PORT, x86arch::PIC_ICW1BASE | x86arch::PIC_NEEDICW4);
            __outb(x86arch::PIC_SLAVE_CMD_PORT, x86arch::PIC_ICW1BASE | x86arch::PIC_NEEDICW4);

            __outb(x86arch::PIC_MASTER_IMR_PORT, 0x20);
            __outb(x86arch::PIC_SLAVE_IMR_PORT, 0x28);

            __outb(x86arch::PIC_MASTER_IMR_PORT, x86arch::PIC_MASTER_SLAVE_LINE);
            __outb(x86arch::PIC_SLAVE_IMR_PORT, x86arch::PIC_SLAVE_ID);

            __outb(x86arch::PIC_MASTER_IMR_PORT, x86arch::PIC_86MODE);
            __outb(x86arch::PIC_SLAVE_IMR_PORT, x86arch::PIC_86MODE);

            __outb(x86arch::PIC_MASTER_IMR_PORT, 0x00);
            __outb(x86arch::PIC_SLAVE_IMR_PORT, 0x00);
        }
    }

    pub fn isr_tab(&mut self) -> u64 {
        let raw = self.isr_table as *mut Buffer;
        return raw as u64;
    }
}

fn __default_unexpected_handler(vector:i32, code:i32) {
    unsafe { asm!("CLI") };
    println!("\nVector: {:X}, Code: {:X}", vector, code);
    loop {}
    //Panic
}

#[no_mangle]
fn __default_expected_handler(vector:i32, code:i32) {
    println!("\nVector: {:X}, Code: {:X}", vector, code);
    if vector >= 0x20 && vector < 0x30 {
        unsafe { __outb(x86arch::PIC_MASTER_CMD_PORT, x86arch::PIC_EOI) };
        if vector > 0x27 {
            unsafe { __outb(x86arch::PIC_SLAVE_CMD_PORT, x86arch::PIC_EOI) };
        }
    }
    else {
        println!("\nVector: {:X}, Code: {:X}", vector, code);
        // Panic
    }
}

fn __default_mystery_handler(vector:i32, code:i32) {
    println!("\nVector: {:X}, Code: {:X}", vector, code);
    unsafe {__outb(x86arch::PIC_MASTER_CMD_PORT, x86arch::PIC_EOI)};
}

lazy_static! {
    pub static ref INT: Mutex<Interrupt> = Mutex::new(Interrupt {
        isr_table: unsafe { &mut *(_kmalloc(ISR_TAB_SIZE * 8) as *mut Buffer) },
    });
}

#[no_mangle]
pub fn get_isr_table() -> u64 {
    let ret = INT.lock().isr_tab();
    //println!("{:X}", ret);
    return ret;
}


pub fn __init_interrupts() {
    INT.lock().init_idt();
    INT.lock().init_pic();
}
