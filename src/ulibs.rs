extern "C" {
    #[no_mangle]
    fn exit();
    #[no_mangle]
    fn fork();
    #[no_mangle]
    fn exec();
    #[no_mangle]
    fn time() -> u64;
    #[no_mangle]
    fn pid() -> u16;
    #[no_mangle]
    fn ppid() -> u16;
}

pub fn sys_exit() {
    unsafe { exit() };
}

pub fn sys_fork() {
    unsafe { fork() };
}

pub fn sys_exec() {
    unsafe { exec() };
}

#[no_mangle]
pub fn sys_time() -> u64 {
    let time = unsafe { time() };
    return time;
}

pub fn sys_pid() -> u16 {
    return unsafe { pid() };
}

pub fn sys_ppid() -> u16 {
    return unsafe { ppid() };
}
