#include <stddef.h>

static long syscall1(long number, long arg1) {
    long ret;
    asm volatile("syscall" : "=a"(ret) : "a"(number), "D"(arg1) : "rcx", "r11", "memory");
    return ret;
}

static long syscall3(long number, long arg1, long arg2, long arg3) {
    long ret;
    asm volatile("syscall" : "=a"(ret) : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3) : "rcx", "r11", "memory");
    return ret;
}

enum {
    SYS_write = 1,
    SYS_exit = 60
};

__attribute__((noreturn))
void _start(void) {
    static const char message[] = "hello from C inside my-os\n";
    syscall3(SYS_write, 1, (long)message, (long)(sizeof(message) - 1));
    syscall1(SYS_exit, 0);
    __builtin_unreachable();
}
