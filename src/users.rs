use crate::println;
use crate::print;
use crate::ulibs;

#[no_mangle]
pub fn init() -> i32 {
    let pid = ulibs::sys_pid();
    let ppid = ulibs::sys_ppid();
    println!("pid {}, ppid {}",pid,ppid);

    let entry = (idle as *mut fn()->i32) as u64;
    ulibs::spawn(entry);

    loop{
        let time = ulibs::sys_time();
        print!("i");
    }

    return 1;
}

fn idle() -> i32 {
    println!("IDLE");
    let pid = ulibs::sys_pid();
    let ppid = ulibs::sys_ppid();
    println!("pid {}, ppid {}",pid,ppid);
    loop{print!(".")}
}
