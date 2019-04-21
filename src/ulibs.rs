use crate::println;
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

pub fn sys_exit() {
    unsafe { exit() };
}

pub fn sys_fork() -> u16 {
    return unsafe { fork() };
}

pub fn sys_exec(entry:u64) -> u16{
    return unsafe { exec(entry) };
}

#[no_mangle]
pub fn sys_time() -> u64 {
    return unsafe { time() };
}

pub fn sys_pid() -> u16 {
    return unsafe { pid() };
}

pub fn sys_ppid() -> u16 {
    return unsafe { ppid() };
}

pub fn sys_wait() -> u16 {
    return unsafe { wait() };
}

pub fn spawn(entry:u64) {
    let new = sys_fork();
    //println!("{}",new);
    if new != 0 {
        return;
    }
    sys_exec(entry);
}
