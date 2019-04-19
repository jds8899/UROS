use crate::println;
use crate::ulibs;

#[no_mangle]
pub fn init() -> i32 {
    let time = ulibs::sys_time();
    println!("{}", time);
    loop{}

    return 1;
}
