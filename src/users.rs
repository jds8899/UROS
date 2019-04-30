///
/// users.rs
///
/// author: Jonathan Schenk
///
/// This file contains some users to show that the OS actually does stuff
///
////////////////////////////////////////////////////////////////////////////////

use crate::uprintln;
use crate::uprint;
use crate::ulibs;

///
/// init
/// Description: The init process. Spawns the idle process and a user.
/// Returns: status, although nothing ever picks this up :/
///
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

///
/// idle
/// Description: The idle process. Runs when nothing else is.
/// Returns: status, although nothing ever picks this up :/
///
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

///
/// user_a
/// Description: A user process. Prints a bunch of 'a's and exits.
/// Returns: status, although nothing ever picks this up :/
///
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
