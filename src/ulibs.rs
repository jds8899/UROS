///
/// ulibs.rs
///
/// Author: Jonathan Schenk
///
/// This file mostly contains syscalls for the users (and spawn)
///
////////////////////////////////////////////////////////////////////////////////

use crate::println;
use crate::print;
use core::fmt;

/// All the syscall stubs we need
/// We cheat by tricking Rust into thinking some of these actually return stuff
extern "C" {
    #[no_mangle]
    fn exit();
    #[no_mangle]
    fn fork() -> u16;
    #[no_mangle]
    fn exec(entry:u64) -> u16;
    #[no_mangle]
    fn time() -> u64;
    #[no_mangle]
    fn pid() -> u16;
    #[no_mangle]
    fn ppid() -> u16;
    #[no_mangle]
    fn wait() -> u16;
}

///
/// sys_exit - terminate the calling process
///
/// Should take an exit status, but does not.
///
pub fn sys_exit() {
    unsafe { exit() };
}

///
/// sys_fork - create a new process
///
/// usage: let pid = sys_fork();
///
/// Returns:
///     parent - pid of spawned process
///     child - 0
///
pub fn sys_fork() -> u16 {
    return unsafe { fork() };
}

///
/// sys_exec - replace this program with a different one
///
/// usage: sys_exit(entry)
///
/// Returns:
///     Supposed to return on failure but doesn't? Just hope it does not fail.
///
pub fn sys_exec(entry:u64) -> u16{
    return unsafe { exec(entry) };
}

///
/// sys_time - get current system time
///
/// usage: let t = sys_time()
///
/// Returns:
///     current system time
///
#[no_mangle]
pub fn sys_time() -> u64 {
    return unsafe { time() };
}

///
/// sys_pid - get PID of this process
///
/// usage: let pid = sys_pid()
///
/// Returns:
///     current proc's pid
///
pub fn sys_pid() -> u16 {
    return unsafe { pid() };
}

///
/// sys_ppid - get PPID of this process
///
/// usage: let ppid = sys_ppid()
///
/// Returns:
///     current proc's ppid
///
pub fn sys_ppid() -> u16 {
    return unsafe { ppid() };
}

///
/// sys_wait - wait for child to terminate
///
/// usage: let pid = wait()
///
/// if there are one or more children in the system and at least one has
/// terminated but hasn't yet been cleaned up, cleans up that process and
/// returns its information; otherwise, blocks until a child terminates
///
/// Returns:
///     Only the child's pid. Not status :(
///
pub fn sys_wait() -> u16 {
    return unsafe { wait() };
}

///
/// sys_spawn - an easier to use amalgamation of fork/exec.
///
/// usage: spawn(entry);
///
/// Returns: Nothin
///
pub fn spawn(entry:u64) {
    let new = sys_fork();
    //println!("{}",new);
    if new != 0 {
        return;
    }
    sys_exec(entry);
}
