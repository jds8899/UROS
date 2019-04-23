use core::ptr;
use core::fmt;
use spin::Mutex;
use lazy_static::lazy_static;

extern "C" {
    fn __outb(port:i32, value:i32);
}

const SCREEN_MIN_X:  u32 = 0;
const SCREEN_MIN_Y:  u32 = 0;
const SCREEN_SIZE_X: u32 = 80;
const SCREEN_SIZE_Y: u32 = 25;
const SCREEN_MAX_X: u32 = SCREEN_SIZE_X - 1;
const SCREEN_MAX_Y: u32 = SCREEN_SIZE_Y - 1;

pub struct Cio {
    scroll_min_x: u32,
    scroll_min_y: u32,
    scroll_max_x: u32,
    scroll_max_y: u32,
    curr_x: u32,
    curr_y: u32,
    min_x: u32,
    min_y: u32,
    max_x: u32,
    max_y: u32,
    buffer: &'static mut Buffer,
}

struct Buffer {
    data: [[u16; (SCREEN_SIZE_X as usize)]; (SCREEN_SIZE_Y as usize)],
}

impl Cio {
    fn bound(min:u32, val:u32, max:u32) -> u32 {
        let mut ret = val;
        if ret < min {
            ret = min;
        }
        if ret > max {
            ret = max;
        }
        return ret;
    }

    fn __c_putchar_at(&mut self, x:u32, y:u32, c:u8) {
        if x < self.max_x && y <= self.max_y {
            let addr = &mut self.buffer.data[y as usize][x as usize] as *mut u16;
            let o_char = c as u16;
            unsafe { ptr::write_volatile(addr, o_char | 0x0700) };
        }
    }

    fn __c_setcursor(&mut self) {
        let mut y = self.curr_y;

        if y > self.scroll_max_y {
            y = self.scroll_max_y;
        }

        let addr = (y as i32) * (SCREEN_SIZE_X as i32) + (self.curr_x as i32) as i32;

        unsafe {
            __outb(0x3d4, 0xe);
            __outb(0x3d5, (addr >> 8) & 0xff);
            __outb(0x3d4, 0xf);
            __outb(0x3d5, addr & 0xff);
        }
    }

    pub fn c_setscroll(&mut self, s_min_x:u32, s_min_y:u32, s_max_x:u32, s_max_y:u32) {
        self.scroll_min_x = Cio::bound(self.min_x, s_min_x, self.max_x);
        self.scroll_min_y = Cio::bound(self.min_y, s_min_y, self.max_y);
        self.scroll_max_x = Cio::bound(self.scroll_min_x, s_max_x, self.max_x);
        self.scroll_max_y = Cio::bound(self.scroll_min_y, s_max_y, self.max_y);
        self.curr_x       = self.scroll_min_x;
        self.curr_y       = self.scroll_min_y;
        self.__c_setcursor();
    }

    pub fn c_moveto(&mut self, x:u32, y:u32) {
        self.curr_x = Cio::bound(self.scroll_min_x, x + self.scroll_min_x, self.scroll_max_x);
        self.curr_y = Cio::bound(self.scroll_min_y, y + self.scroll_min_y, self.scroll_max_y);
        self.__c_setcursor();
    }

    pub fn c_putchar_at(&mut self, mut x:u32, y:u32, c:u8) {
        if (c & 0x7f) == b'\n' {
            let mut limit = 0 as u32;

            if x > self.scroll_max_x {
                limit = self.max_x;
            }
            else if x >= self.scroll_min_x {
                limit = self.scroll_max_x;
            }
            else {
                limit = self.scroll_min_x - 1;
            }
            while x < limit {
                self.__c_putchar_at(x, y, b' ');
                x += 1;
            }
        }
        else {
            self.__c_putchar_at(x, y, c);
        }
    }

    pub fn c_putchar(&mut self, c:u8) {
        if self.curr_y >= self.scroll_max_y {
            let diff = self.curr_y - self.scroll_max_y + 1;
            self.c_scroll(diff);
            self.curr_y = self.scroll_max_y - 1;
        }

        let mut x = self.curr_x;
        let y = self.curr_y;

        match c {
            b'\n' => {
                while self.curr_x <= self.scroll_max_x {
                    self.__c_putchar_at(x, y, b' ');
                    self.curr_x += 1;
                    x = self.curr_x;
                }
                self.curr_x  = self.scroll_min_x;
                self.curr_y += 1;
            }
            b'\r' => {
                self.curr_x = self.scroll_min_x;
            }
            c => {
                self.__c_putchar_at(x, y, c);
                self.curr_x += 1;
                if self.curr_x > self.scroll_max_x {
                    self.curr_x  = self.scroll_min_x;
                    self.curr_y += 1;
                }
            }
        }
        self.__c_setcursor()
    }

    pub fn c_puts_at(&mut self, mut x:u32, mut y:u32, s: &str) {
        for c in s.bytes() {
            if x > self.max_x {continue};
            self.c_putchar_at(x, y, c);
            x += 1;
        }
    }

    pub fn c_puts(&mut self, s: &str) {
        //unsafe { asm!("CLI") };
        for c in s.bytes() {
            self.c_putchar(c);
        }
        //unsafe { asm!("STI") };
    }

    pub fn c_clearscreen(&mut self) {
        for y in 0..self.max_y {
            for x in 0..self.max_x {
                self.__c_putchar_at(x, y, b' ');
            }
        }
    }

    pub fn c_clearscroll(&mut self) {
        for y in self.scroll_min_y..self.scroll_max_y {
            for x in self.scroll_min_x..self.scroll_max_x {
                self.__c_putchar_at(x, y, b' ');
            }
        }
    }

    pub fn c_scroll(&mut self, lines:u32) {
        if lines > self.scroll_max_y - self.scroll_min_y {
            self.c_clearscroll();
            self.curr_x = self.scroll_min_x;
            self.curr_y = self.scroll_min_y;
            self.__c_setcursor();
            return;
        }

        for y in self.scroll_min_y..(self.scroll_max_y - lines) {
            for x in self.scroll_min_x..self.scroll_max_x {
                let from = self.buffer.data[(y + lines) as usize][x as usize];
                let to = &mut self.buffer.data[y as usize][x as usize] as *mut u16;
                unsafe { ptr::write_volatile(to, from) };
            }
        }
        for y in (self.scroll_max_y - lines)..self.scroll_max_y {
            for x in self.scroll_min_x..self.scroll_max_x {
                self.__c_putchar_at(x, y, b' ');
            }
        }
    }
}

impl fmt::Write for Cio {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.c_puts(s);
        Ok(())
    }
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::c_io::_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:tt)*) => ($crate::print!("{}\n", format_args!($($arg)*)));
}

#[macro_export]
macro_rules! uprint {
    ($($arg:tt)*) => ($crate::c_io::_uprint(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! uprintln {
    () => ($crate::uprint!("\n"));
    ($($arg:tt)*) => ($crate::uprint!("{}\n", format_args!($($arg)*)));
}

#[doc(hidden)]
pub fn _uprint(args: fmt::Arguments) {
    use core::fmt::Write;
    unsafe { asm!("CLI") };
    WRITER.lock().write_fmt(args).unwrap();
    unsafe { asm!("STI") };
}

#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    use core::fmt::Write;
    WRITER.lock().write_fmt(args).unwrap();
}

lazy_static! {
    pub static ref WRITER: Mutex<Cio> = Mutex::new(Cio {
        scroll_min_x: 0,
        scroll_min_y: 0,
        scroll_max_x: 80,
        scroll_max_y: 25,
        curr_x: 0,
        curr_y: 0,
        min_x: 0,
        min_y: 0,
        max_x: 80,
        max_y: 25,
        buffer: unsafe { &mut *(0xb8000 as *mut Buffer) },
    });
}

pub fn cio_test() {

    WRITER.lock().c_moveto(0, 0);
    WRITER.lock().c_putchar(b'F');
    //WRITER.lock().c_puts("hello\n");
    WRITER.lock().c_setscroll(0,5, 80, 25);
    WRITER.lock().c_clearscroll();
    WRITER.lock().c_moveto(0,19);
    WRITER.lock().c_puts("hello\n");
    WRITER.lock().c_puts("hello");
    //WRITER.lock().c_scroll(5);
    //WRITER.lock().c_clearscreen();
    //writer.__put_char_at(20, 10, b'F');
    //writer.__c_setcursor();
}
