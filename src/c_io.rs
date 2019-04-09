//#[macro_use]
//extern crate lazy_static;

use core::ptr;

const SCREEN_MIN_X:  usize = 0;
const SCREEN_MIN_Y:  usize = 0;
const SCREEN_SIZE_X: usize = 80;
const SCREEN_SIZE_Y: usize = 25;
const SCREEN_MAX_X: usize = SCREEN_SIZE_X - 1;
const SCREEN_MAX_Y: usize = SCREEN_SIZE_Y - 1;

pub struct Cio {
    scroll_min_x: usize,
    scroll_min_y: usize,
    scroll_max_x: usize,
    scroll_max_y: usize,
    scroll_curr_x: usize,
    scroll_curr_y: usize,
    min_x: usize,
    min_y: usize,
    max_x: usize,
    max_y: usize,
    buffer: &'static mut Buffer,
}

struct Buffer {
    data: [[u16; SCREEN_SIZE_X]; SCREEN_SIZE_Y],
}

impl Cio {
    pub fn __put_char_at(&mut self, x:usize, y:usize, c:u8) {
        let addr = &mut self.buffer.data[x][y] as *mut u16;
        let o_char = c as u16;
        if c > 0xff {
            unsafe { ptr::write_volatile(addr, o_char) };
        }
        else {
            unsafe { ptr::write_volatile(addr, o_char | 0x0700) };
        }
    }
}

pub fn print_stuff() {
    let mut writer = Cio {
        scroll_min_x: 0,
        scroll_min_y: 0,
        scroll_max_x: 80,
        scroll_max_y: 25,
        scroll_curr_x: 0,
        scroll_curr_y: 0,
        min_x: 0,
        min_y: 0,
        max_x: 80,
        max_y: 25,
        buffer: unsafe { &mut *(0xb8000 as *mut Buffer) },
    };

    writer.__put_char_at(20, 10, b'F');
}
