#include <stddef.h>
#include <stdint.h>

struct timespec {
    long tv_sec;
    long tv_nsec;
};

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

enum {
    SYS_read = 0,
    SYS_write = 1,
    SYS_close = 3,
    SYS_getcwd = 79,
    SYS_chdir = 80,
    SYS_uname = 63,
    SYS_openat = 257,
    SYS_getdents64 = 217,
    SYS_reboot = 169
};

enum {
    REBOOT_MAGIC1 = 0xfee1dead,
    REBOOT_MAGIC2 = 0x28121969,
    REBOOT_CMD_RESTART = 0x1234567,
    REBOOT_CMD_HALT = 0xcdef0123,
    REBOOT_CMD_POWER_OFF = 0x4321fedc
};

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

static long syscall4(long number, long arg1, long arg2, long arg3, long arg4) {
    long ret;
    register long r10 asm("r10") = arg4;
    asm volatile("syscall" : "=a"(ret) : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10) : "rcx", "r11", "memory");
    return ret;
}

static long syscall5(long number, long arg1, long arg2, long arg3, long arg4, long arg5) {
    long ret;
    register long r10 asm("r10") = arg4;
    register long r8 asm("r8") = arg5;
    asm volatile("syscall" : "=a"(ret) : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
    return ret;
}

static long write_str(const char* string) {
    long len = 0;
    while (string[len] != '\0') {
        len++;
    }
    return syscall3(SYS_write, 1, (long)string, len);
}

static long write_buf(const char* buffer, long len) {
    return syscall3(SYS_write, 1, (long)buffer, len);
}

static long read_line(char* buffer, long capacity) {
    long length = 0;
    while (length < capacity - 1) {
        char ch;
        long result = syscall3(SYS_read, 0, (long)&ch, 1);
        if (result <= 0) {
            break;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            write_str("\n");
            break;
        }
        if (ch == 0x7f || ch == '\b') {
            if (length > 0) {
                length--;
                write_str("\b \b");
            }
            continue;
        }
        buffer[length++] = ch;
        write_buf(&ch, 1);
    }
    buffer[length] = '\0';
    return length;
}

static int string_equals(const char* left, const char* right) {
    long i = 0;
    while (left[i] != '\0' && right[i] != '\0') {
        if (left[i] != right[i]) {
            return 0;
        }
        i++;
    }
    return left[i] == right[i];
}

static char* skip_spaces(char* string) {
    while (*string == ' ' || *string == '\t') {
        string++;
    }
    return string;
}

static long parse_arguments(char* line, char* argv[], long max_args) {
    long argc = 0;
    char* cursor = skip_spaces(line);
    while (*cursor != '\0' && argc < max_args) {
        argv[argc++] = cursor;
        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t') {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }
        *cursor = '\0';
        cursor = skip_spaces(cursor + 1);
    }
    return argc;
}

static void print_error(const char* command, const char* reason) {
    write_str("mysh: ");
    write_str(command);
    write_str(": ");
    write_str(reason);
    write_str("\n");
}

static void command_help(void) {
    write_str(
        "Commands:\n"
        "  help        Show this message\n"
        "  clear       Clear the screen\n"
        "  echo        Print arguments\n"
        "  pwd         Print current directory\n"
        "  cd PATH     Change directory\n"
        "  ls [PATH]   List directory contents\n"
        "  cat FILE    Print a file\n"
        "  uname       Show kernel information\n"
        "  reboot      Reboot the machine\n"
        "  poweroff    Power off the machine\n"
        "  halt        Halt the machine\n"
    );
}

static void command_clear(void) {
    write_str("\033[2J\033[H");
}

static void command_echo(char* argv[], long argc) {
    for (long i = 1; i < argc; i++) {
        if (i > 1) {
            write_str(" ");
        }
        write_str(argv[i]);
    }
    write_str("\n");
}

static void command_pwd(void) {
    char buffer[256];
    long result = syscall3(SYS_getcwd, (long)buffer, (long)sizeof(buffer), 0);
    if (result < 0) {
        print_error("pwd", "unable to read current directory");
        return;
    }
    write_str(buffer);
    write_str("\n");
}

static void command_cd(char* argv[], long argc) {
    const char* target = argc > 1 ? argv[1] : "/";
    long result = syscall1(SYS_chdir, (long)target);
    if (result < 0) {
        print_error("cd", "directory not found");
    }
}

static void print_file(const char* path) {
    long fd = syscall4(SYS_openat, -100, (long)path, 0, 0);
    if (fd < 0) {
        print_error("cat", "file not found");
        return;
    }

    char buffer[512];
    for (;;) {
        long count = syscall3(SYS_read, fd, (long)buffer, (long)sizeof(buffer));
        if (count < 0) {
            print_error("cat", "read error");
            break;
        }
        if (count == 0) {
            break;
        }
        write_buf(buffer, count);
    }

    syscall1(SYS_close, fd);
}

static void command_cat(char* argv[], long argc) {
    if (argc < 2) {
        print_error("cat", "missing file operand");
        return;
    }
    print_file(argv[1]);
}

static void command_uname(void) {
    struct utsname info;
    long result = syscall1(SYS_uname, (long)&info);
    if (result < 0) {
        print_error("uname", "kernel refused request");
        return;
    }
    write_str(info.sysname);
    write_str(" ");
    write_str(info.release);
    write_str(" ");
    write_str(info.machine);
    write_str("\n");
}

static void list_directory(const char* path) {
    long fd = syscall4(SYS_openat, -100, (long)path, 0, 0);
    if (fd < 0) {
        print_error("ls", "directory not found");
        return;
    }

    char buffer[1024];
    for (;;) {
        long count = syscall3(SYS_getdents64, fd, (long)buffer, (long)sizeof(buffer));
        if (count < 0) {
            print_error("ls", "cannot read directory");
            break;
        }
        if (count == 0) {
            break;
        }

        long offset = 0;
        while (offset < count) {
            struct linux_dirent64* entry = (struct linux_dirent64*)(buffer + offset);
            const char* name = entry->d_name;
            if (!(name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))) {
                write_str(name);
                write_str("\n");
            }
            offset += entry->d_reclen;
        }
    }

    syscall1(SYS_close, fd);
}

static void command_ls(char* argv[], long argc) {
    const char* path = argc > 1 ? argv[1] : ".";
    list_directory(path);
}

static void reboot_machine(long command) {
    syscall5(SYS_reboot, REBOOT_MAGIC1, REBOOT_MAGIC2, command, 0, 0);
}

static void command_reboot(void) {
    reboot_machine(REBOOT_CMD_RESTART);
    print_error("reboot", "reboot failed");
}

static void command_poweroff(void) {
    reboot_machine(REBOOT_CMD_POWER_OFF);
    print_error("poweroff", "power off failed");
}

static void command_halt(void) {
    reboot_machine(REBOOT_CMD_HALT);
    print_error("halt", "halt failed");
}

static void run_command(char* argv[], long argc) {
    if (argc == 0) {
        return;
    }

    if (string_equals(argv[0], "help")) {
        command_help();
    } else if (string_equals(argv[0], "clear")) {
        command_clear();
    } else if (string_equals(argv[0], "echo")) {
        command_echo(argv, argc);
    } else if (string_equals(argv[0], "pwd")) {
        command_pwd();
    } else if (string_equals(argv[0], "cd")) {
        command_cd(argv, argc);
    } else if (string_equals(argv[0], "ls")) {
        command_ls(argv, argc);
    } else if (string_equals(argv[0], "cat")) {
        command_cat(argv, argc);
    } else if (string_equals(argv[0], "uname")) {
        command_uname();
    } else if (string_equals(argv[0], "reboot")) {
        command_reboot();
    } else if (string_equals(argv[0], "poweroff")) {
        command_poweroff();
    } else if (string_equals(argv[0], "halt")) {
        command_halt();
    } else {
        print_error(argv[0], "command not found");
    }
}

static void shell_loop(void) {
    char line[256];
    char* argv[32];

    for (;;) {
        write_str("mysh:/# ");
        long length = read_line(line, (long)sizeof(line));
        if (length <= 0) {
            write_str("\n");
            continue;
        }
        long argc = parse_arguments(line, argv, 32);
        run_command(argv, argc);
    }
}

__attribute__((noreturn))
void _start(void) {
    write_str("my-os shell ready\n");
    shell_loop();
    __builtin_unreachable();
}
