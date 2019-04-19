extern "C" {
    #[no_mangle]
    fn exit();
    #[no_mangle]
    fn fork();
    #[no_mangle]
    fn exec();
    #[no_mangle]
    fn time();
    #[no_mangle]
    fn pid();
    #[no_mangle]
    fn ppid();
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

pub fn sys_time() -> u64 {
    unsafe { time() };
    return 0;
}

pub fn sys_pid() {
    unsafe { pid() };
}

pub fn sys_ppid() {
    unsafe { ppid() };
}
