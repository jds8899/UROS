use crate::println;
use crate::ulibs;

#[no_mangle]
pub fn init() -> i32 {
    let pid = ulibs::sys_pid();
    let ppid = ulibs::sys_ppid();
    println!("pid {}, ppid {}",pid,ppid);
    loop{
        //let time = ulibs::sys_time();
        //println!("{}", time);
    }

    return 1;
}
