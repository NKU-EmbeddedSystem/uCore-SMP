#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>
#include <fcntl.h>

int test(char *argv[]) {
    int pid = fork();
    if (pid == 0) {
        // child process
        if (execv(argv[0], argv) < 0) {
            printf("test runner: %s: No such file\n", argv[0]);
            exit(-9);
        }
        panic("unreachable!");
    } else {
        int xstate = 0;
        int exit_pid = 0;
        exit_pid = waitpid(pid, &xstate, 0);
        assert(pid == exit_pid);
        printf("test runner: Process %d exited with code %d\n", pid, xstate);
    }
}

int main() {

    // create /dev directory
    int fd_dir = open("/dev", O_DIRECTORY | O_CREATE);
    if (fd_dir < 0) {
        printf("Shell: can not open /dev\n");
        return -1;
    }
    close(fd_dir);

    if (open("/dev/console", O_RDWR) < 0) {
        mknod("/dev/console", 1, 0);
        open("/dev/console", O_RDWR);
    }

    dup(0); // stdout
    dup(0); // stderr

    if (open("/dev/cpu", O_RDWR) < 0) {
        mknod("/dev/cpu", 2, 0);
        // open("cpu", O_RDWR);
    }
    if (open("/dev/mem", O_RDWR) < 0) {
        mknod("/dev/mem", 3, 0);
        // open("mem", O_RDWR);
    }
    if (open("/dev/proc", O_RDWR) < 0) {
        mknod("/dev/proc", 4, 0);
        // open("proc", O_RDWR);
    }
    if (open("/dev/null", O_RDWR) < 0) {
        mknod("/dev/null", 5, 0);
        // open("null", O_RDWR);
    }
    if (open("/dev/zero", O_RDWR) < 0) {
        mknod("/dev/zero", 6, 0);
        // open("zero", O_RDWR);
    }

    // create /lib directory
    fd_dir = open("/lib", O_DIRECTORY | O_CREATE);
    if (fd_dir < 0) {
        printf("Shell: can not open /lib\n");
        return -1;
    }
    close(fd_dir);

    // create /tmp directory
    fd_dir = open("/tmp", O_DIRECTORY | O_CREATE);
    if (fd_dir < 0) {
        printf("Shell: can not open /tmp\n");
        return -1;
    }
    close(fd_dir);

    // link dynamic library to /lib directory
    link("/libc.so", "/lib/ld-musl-riscv64-sf.so.1");
    link("/libdlopen_dso.so", "/lib/libdlopen_dso.so");
    link("/libtls_align_dso.so", "/lib/libtls_align_dso.so");
    link("/libtls_get_new-dtv_dso.so", "/lib/libtls_get_new-dtv_dso.so");
    link("libtls_init_dso.so", "/lib/libtls_init_dso.so");

    char *exe_file[] = {"entry-static.exe", "entry-dynamic.exe"};
    char *testname[] = {
            "argv",
            "basename",
            "clocale_mbfuncs",
            "clock_gettime",
            "crypt",
            "dirname",
            "env",
            "fdopen",
            "fnmatch",
            "fscanf",
            "fwscanf",
            "iconv_open",
            "inet_pton",
            "mbc",
            "memstream",
            "pthread_cancel_points",
            "pthread_cancel",
            "pthread_cond",
            "pthread_tsd",
            "qsort",
            "random",
            "search_hsearch",
            "search_insque",
            "search_lsearch",
            "search_tsearch",
            "setjmp",
            "snprintf",
            "socket",
            "sscanf",
            "sscanf_long",
            "stat",
            "strftime",
            "string",
            "string_memcpy",
            "string_memmem",
            "string_memset",
            "string_strchr",
            "string_strcspn",
            "string_strstr",
            "strptime",
            "strtod",
            "strtod_simple",
            "strtof",
            "strtol",
            "strtold",
            "swprintf",
            "tgmath",
            "time",
            "tls_align",
            "udiv",
            "ungetc",
            "utime",
            "wcsstr",
            "wcstol",
            "pleval",
            "daemon_failure",
            "dn_expand_empty",
            "dn_expand_ptr_0",
            "fflush_exit",
            "fgets_eof",
            "fgetwc_buffering",
            "flockfile_list",
            "fpclassify_invalid_ld80",
            "ftello_unflushed_append",
            "getpwnam_r_crash",
            "getpwnam_r_errno",
            "iconv_roundtrips",
            "inet_ntop_v4mapped",
            "inet_pton_empty_last_field",
            "iswspace_null",
            "lrand48_signextend",
            "lseek_large",
            "malloc_0",
            "mbsrtowcs_overflow",
            "memmem_oob_read",
            "memmem_oob",
            "mkdtemp_failure",
            "mkstemp_failure",
            "printf_1e9_oob",
            "printf_fmt_g_round",
            "printf_fmt_g_zeros",
            "printf_fmt_n",
            "pthread_robust_detach",
            "pthread_cancel_sem_wait",
            "pthread_cond_smasher",
            "pthread_condattr_setclock",
            "pthread_exit_cancel",
            "pthread_once_deadlock",
            "pthread_rwlock_ebusy",
            "putenv_doublefree",
            "regex_backref_0",
            "regex_bracket_icase",
            "regex_ere_backref",
            "regex_escaped_high_byte",
            "regex_negated_range",
            "regexec_nosub",
            "rewind_clear_error",
            "rlimit_open_files",
            "scanf_bytes_consumed",
            "scanf_match_literal_eof",
            "scanf_nullbyte_char",
            "setvbuf_unget",
            "sigprocmask_internal",
            "sscanf_eof",
            "statvfs",
            "strverscmp",
            "syscall_sign_extend",
            "uselocale_0",
            "wcsncpy_read_overflow",
            "wcsstr_false_negative"
    };

    for (int i = 0; i < sizeof(exe_file) / sizeof(char *); i++) {
        for (int j = 0; j < sizeof(testname) / sizeof(char *); j++) {
            char *argv[] = {"runtest.exe", "-w", exe_file[i], testname[j], NULL};
            test(argv);
        }
    }

    return 0;
}
