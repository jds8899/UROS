use crate::println;

#[no_mangle]
pub fn init() -> i32 {
    loop{println!("hi")}

    return 1;
}
