use crate::println;
use crate::print;
use crate::ulibs;

#[no_mangle]
pub fn init() -> i32 {
    println!("Spawning Idle");
    let entry = (idle as *mut fn()->i32) as u64;
    ulibs::spawn(entry);

    println!("Spawning A");
    let entry2 = (user_a as *mut fn()->i32) as u64;
    ulibs::spawn(entry2);

    let pid = ulibs::sys_pid();
    let ppid = ulibs::sys_ppid();
    println!("pid {}, ppid {}",pid,ppid);
    loop{
        let whom = ulibs::sys_wait();
        println!("{} exited", whom);
    }

    return 1;
}

fn idle() -> i32 {
    println!("IDLE");
    let pid = ulibs::sys_pid();
    let ppid = ulibs::sys_ppid();
    println!("pid {}, ppid {}",pid,ppid);
    loop{
        let time = ulibs::sys_time();
        if time % 0x100 == 0 {
            print!(".")
        }
    }
}

fn user_a() -> i32 {
    for i in 0..10000 {
        let time = ulibs::sys_time();
        if time % 0x100 == 0 {
            print!("a")
        }
    }
    println!();
    return 0;
}
