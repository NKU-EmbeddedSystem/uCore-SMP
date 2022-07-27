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
        exit_pid = waitpid(pid, &xstate,0);
        assert(pid == exit_pid);
        printf("test runner: Process %d exited with code %d\n", pid, xstate);
    }
}

int main() {

    if (open("console", O_RDWR) < 0) {
        mknod("console", 1, 0);
        open("console", O_RDWR);
    }

    dup(0); // stdout
    dup(0); // stderr

    if (open("cpu", O_RDWR) < 0) {
        mknod("cpu", 2, 0);
        // open("cpu", O_RDWR);
    }
    if (open("mem", O_RDWR) < 0) {
        mknod("mem", 3, 0);
        // open("mem", O_RDWR);
    }
    if (open("proc", O_RDWR) < 0) {
        mknod("proc", 4, 0);
        // open("proc", O_RDWR);
    }

    int fd_dir = open("/lib", O_DIRECTORY | O_CREATE);
    if (fd_dir < 0) {
        printf("Shell: can not open /lib\n");
        return -1;
    }
    close(fd_dir);
    link("/libc.so", "/lib/ld-musl-riscv64-sf.so.1");
    link("/libdlopen_dso.so", "/lib/libdlopen_dso.so");
    link("/libtls_align_dso.so", "/lib/libtls_align_dso.so");
    link("/libtls_get_new-dtv_dso.so", "/lib/libtls_get_new-dtv_dso.so");
    link("libtls_init_dso.so", "/lib/libtls_init_dso.so");

    char *exe_file[] = {"entry-static.exe", "entry-dynamic.exe"};
    char *testname[] = {"argv"};

    for (int i = 0; i < sizeof(exe_file) / sizeof(char *); i++) {
        for (int j = 0; j < sizeof(testname) / sizeof(char *); j++) {
            char *argv[] = {"runtest.exe", "-w", exe_file[i], testname[j], NULL};
            test(argv);
        }
    }

    return 0;
}
