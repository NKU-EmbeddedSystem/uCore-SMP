#include <stdio.h>
#include <stdlib.h>
#include <ucore.h>
#include <string.h>
#include <fcntl.h>

int test(char *argv[]) {
    char *envp[] = {"PATH=.", NULL};
    int pid = fork();
    if (pid == 0) {
        // child process
        if (execve(argv[0], argv, envp) < 0) {
            printf("test runner: %s: No such file\n", argv[0]);
            exit(-9);
        }
        panic("unreachable!");
    } else {
        int wstatus = 0;
        int exit_pid = 0;
        exit_pid = waitpid(pid, &wstatus, 0);
        assert(pid == exit_pid);
        return WEXITSTATUS(wstatus);
    }
}

int test_lua() {
    char *argv[] = {
            "lua",
            NULL, // script file
            NULL
    };
    char *testcases[] = {
            "date.lua",
            "file_io.lua",
            "max_min.lua",
            "random.lua",
            "remove.lua",
            "round_num.lua",
            "sin30.lua",
            "sort.lua",
            "strings.lua",
    };
    for (int i = 0; i < sizeof(testcases) / sizeof(char *); i++) {
        argv[1] = testcases[i];
        int ret = test(argv);
        if (ret == 0) {
            printf("testcase lua %s success\n", testcases[i]);
        } else {
            printf("testcase lua %s fail\n", testcases[i]);
        }
    }
}

// parse command line, including "
int parse_line(char *line, char *argv[]) {
    int argc = 0;
    char *p = line;
    while (*p != '\0') {
        while (*p == ' ') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p != '\0' && *p != '"') {
                p++;
            }
            if (*p == '"') {
                *p = '\0';
                p++;
            }
        } else {
            argv[argc++] = p;
            while (*p != '\0' && *p != ' ') {
                p++;
            }
            if (*p == ' ') {
                *p = '\0';
                p++;
            }
        }
    }
    argv[argc] = NULL;
    return argc;
}

int test_busybox_read_testcases(char *testcases_buf, char *testcases[]) {
    int fd = open("busybox_cmd.txt", O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    int n = read(fd, testcases_buf, 1024 - 1);
    testcases_buf[n] = '\0';

    // split by \n, and write to testcases
    char *p = testcases_buf;
    int i = 0;
    while (*p != '\0') {
        if (*p == '\n') {
            *p = '\0';
            p++;
            testcases[i++] = testcases_buf;
            testcases_buf = p;
        } else {
            p++;
        }
    }
    close(fd);
    testcases[i] = NULL;
    return i;
}

int test_busybox() {
    char *argv[10] = {"busybox", "sh", "-c"}; // up to 10 arguments
    char testcases_buf[1024];
    char *testcases[100] = {0};
    int n = test_busybox_read_testcases(testcases_buf, testcases);

    for (int i = 0; i < n; i++) {
        char line[256];
        strcpy(line, "./busybox ");
        strcat(line, testcases[i]);
        argv[3] = line;
        int ret = test(argv);
        if (ret == 0 || strcmp(testcases[i], "false") == 0) {
            printf("testcase busybox %s success\n", testcases[i]);
        } else {
            printf("testcase busybox %s fail\n", testcases[i]);
        }
    }
}

void run_busybox() {
    char *argv[10] = {"busybox", "sh", NULL};
    test(argv);
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

//    // create /lib directory
//    fd_dir = open("/lib", O_DIRECTORY | O_CREATE);
//    if (fd_dir < 0) {
//        printf("Shell: can not open /lib\n");
//        return -1;
//    }
//    close(fd_dir);
//
//    // create /tmp directory
//    fd_dir = open("/tmp", O_DIRECTORY | O_CREATE);
//    if (fd_dir < 0) {
//        printf("Shell: can not open /tmp\n");
//        return -1;
//    }
//    close(fd_dir);
//
//    // link dynamic library to /lib directory
//    link("/libc.so", "/lib/ld-musl-riscv64-sf.so.1");
//    link("/libdlopen_dso.so", "/lib/libdlopen_dso.so");
//    link("/libtls_align_dso.so", "/lib/libtls_align_dso.so");
//    link("/libtls_get_new-dtv_dso.so", "/lib/libtls_get_new-dtv_dso.so");
//    link("libtls_init_dso.so", "/lib/libtls_init_dso.so");

//    test_lua();
    test_busybox();
//    run_busybox();
    return 0;
}
