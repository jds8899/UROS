use crate::uprintln;
use crate::uprint;
use crate::ulibs;

#[no_mangle]
pub fn init() -> i32 {
    uprintln!("Spawning Idle");
    let entry = (idle as *mut fn()->i32) as u64;
    ulibs::spawn(entry);

    uprintln!("Spawning A");
    let entry2 = (user_a as *mut fn()->i32) as u64;
    ulibs::spawn(entry2);

    let pid = ulibs::sys_pid();
    let ppid = ulibs::sys_ppid();
    uprintln!("pid {}, ppid {}",pid,ppid);
    loop{
        let whom = ulibs::sys_wait();
        uprintln!("Init reporting {} exited", whom);
    }

    return 1;
}

fn idle() -> i32 {
    uprintln!("IDLE");
    let pid = ulibs::sys_pid();
    let ppid = ulibs::sys_ppid();
    uprintln!("pid {}, ppid {}",pid,ppid);
    loop{
        let time = ulibs::sys_time();
        if time % 0x10000 == 0 {
            uprint!(".")
        }
    }
}

fn user_a() -> i32 {
    for i in 0..10000 {
        let time = ulibs::sys_time();
        if time % 0x10 == 0 {
            uprint!("a")
        }
    }
    uprintln!();
    return 0;
}
