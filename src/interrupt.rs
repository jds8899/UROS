use core::ptr;
use core::ffi;
use spin::Mutex;
use lazy_static::lazy_static;
use crate::println;

extern "C" {
    #[no_mangle]
    fn __outb(port:i32, value:i32);
    #[no_mangle]
    fn _kmalloc(size:u64) -> usize;
    #[no_mangle]
    static __isr_stub_table: usize;
    const INT_VEC_KEYBOARD: usize;
    const INT_VEC_TIMER: usize;
    const INT_VEC_MYSTERY: usize;
    const IDT_ADDRESS: usize;
    const IDT_PRESENT: usize;
    const IDT_DPL_0: usize;
    const IDT_INT32_GATE: usize;
}

const ISR_TAB_USIZE: usize = 256;
const ISR_TAB_SIZE: u64 = 256;

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
        for i in 0..ISR_TAB_USIZE {
            self.set_idt_entry(i, __isr_stub_table + (8 * i));
            self.__install_isr(i, __default_unexpected_handler);
        }

        self.__install_isr(INT_VEC_KEYBOARD, __default_expected_handler);
        self.__install_isr(INT_VEC_TIMER, __default_expected_handler);
        self.__install_isr(INT_VEC_MYSTERY, __default_mystery_handler);
    }

    pub fn set_idt_entry(&mut self, entry:usize, handler:usize) {
        let g = unsafe { &mut *(IDT_ADDRESS + entry) as *mut idt_gate };

        g.offset_15_0 = handler & 0xffff as u16;
        g.segment_selector = 0x0008;
        g.flags = IDT_PRESENT | IDT_DPL_0 | IDT_INT32_GATE as u16;
        g.offset_31_16 = (handler >> 16) & 0xffff as u16;
        g.offset_63_32 = (handler >> 32) & 0xffffffff as u32;
        g.zero = 0x00000000;
    }

    pub fn __install_isr(vector:usize, handler:fn(i32, i32)) -> fn(i32, i32) {
        let old_handler = self.isr_table.data[vector];
        self.isr_table.data[vector] = handler;
        return old_handler;
    }
}

lazy_static! {
    static ref INT: Mutex<Interrupt> = Mutex::new(Interrupt {
        isr_table: unsafe { &mut *(_kmalloc(ISR_TAB_SIZE * 8) as *mut Buffer) },
    });
}

pub fn __init_interrupts() {
}
