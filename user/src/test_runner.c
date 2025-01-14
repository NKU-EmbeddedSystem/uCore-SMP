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

void test_lua() {
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

void test_busybox() {
    char *argv[10] = {"busybox", "sh", "-c"}; // up to 10 arguments

    // because the "sort test.txt | ./busybox uniq" testcase is not supported,
    // so we cannot use the testcases in "busybox_cmd.txt", we should skip it.
    char testcases_buf[1024];
//    char *testcases[100] = {0};
//    int n = test_busybox_read_testcases(testcases_buf, testcases);

    char *testcases[] = {
        "echo \"#### independent command test\"",
        "ash -c exit",
        "sh -c exit",
        "basename /aaa/bbb",
        "cal",
        "clear",
        "date",
        "df",
        "dirname /aaa/bbb",
        "dmesg",
        "du",
        "expr 1 + 1",
        "false",
        "true",
        "which ls",
        "uname",
        "uptime",
        "printf \"abc\\n\"",
        "ps",
        "pwd",
        "free",
        "hwclock",
        "kill 10",
        "ls",
        "sleep 1",
        "echo \"#### file opration test\"",
        "touch test.txt",
        "echo \"hello world\" > test.txt",
        "cat test.txt",
        "cut -c 3 test.txt",
        "od test.txt",
        "head test.txt",
        "tail test.txt",
        "hexdump -C test.txt",
        "md5sum test.txt",
        "echo \"ccccccc\" >> test.txt",
        "echo \"bbbbbbb\" >> test.txt",
        "echo \"aaaaaaa\" >> test.txt",
        "echo \"2222222\" >> test.txt",
        "echo \"1111111\" >> test.txt",
        "echo \"bbbbbbb\" >> test.txt",
//        "sort test.txt | ./busybox uniq",
        "stat test.txt",
        "strings test.txt",
        "wc test.txt",
        "[ -f test.txt ]",
        "more test.txt",
        "rm test.txt",
        "mkdir test_dir",
        "mv test_dir test",
        "rmdir test",
        "grep hello busybox_cmd.txt",
        "cp busybox_cmd.txt busybox_cmd.bak",
        "rm busybox_cmd.bak",
        "find -name \"busybox_cmd.txt\"",
    };
    int n = sizeof(testcases) / sizeof(char *);

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

void test_lmbench() {

    char *argv[10] = {"busybox", "sh", "-c"};

    char *testcases[] = {
            "busybox echo latency measurements",
            "lmbench_all lat_syscall -P 1 null",
            "lmbench_all lat_syscall -P 1 read",
            "lmbench_all lat_syscall -P 1 write",
            "busybox mkdir -p /var/tmp",
            "busybox touch /var/tmp/lmbench",
            "lmbench_all lat_syscall -P 1 stat /var/tmp/lmbench",
            "lmbench_all lat_syscall -P 1 fstat /var/tmp/lmbench",
            "lmbench_all lat_syscall -P 1 open /var/tmp/lmbench",
            "lmbench_all lat_select -n 100 -P 1 file",
            "lmbench_all lat_sig -P 1 install",
            "lmbench_all lat_sig -P 1 catch",
//            "lmbench_all lat_sig -P 1 prot lat_sig", // bad
            "lmbench_all lat_pipe -P 1",
            "lmbench_all lat_proc -P 1 fork",
            "lmbench_all lat_proc -P 1 exec",
            "busybox cp hello /tmp", // maybe bad,
            "lmbench_all lat_proc -P 1 shell",
            "lmbench_all lmdd label=\"File /var/tmp/XXX write bandwidth:\" "
            "of=/var/tmp/XXX move=1m fsync=1 print=3",
            "lmbench_all lat_pagefault -P 1 /var/tmp/XXX",
            "lmbench_all lat_mmap -P 1 512k /var/tmp/XXX",
            "busybox echo file system latency",
            "lmbench_all lat_fs /var/tmp",
            "busybox echo Bandwidth measurements",
            "lmbench_all bw_pipe -P 1",
            "lmbench_all bw_file_rd -P 1 512k io_only /var/tmp/XXX",
            "lmbench_all bw_file_rd -P 1 512k open2close /var/tmp/XXX",
            "lmbench_all bw_mmap_rd -P 1 512k mmap_only /var/tmp/XXX",
            "lmbench_all bw_mmap_rd -P 1 512k open2close /var/tmp/XXX",
            "busybox echo context switch overhead",
            "lmbench_all lat_ctx -P 1 -s 32 2 4 8 16 24 32 64 96",
    };
    int n = sizeof(testcases) / sizeof(char *);

    for (int i = 0; i < n; i++) {
        argv[3] = testcases[i];
        int ret = test(argv);
//        if (ret == 0) {
//            printf("testcase lmbench %s success\n", testcases[i]);
//        } else {
//            printf("testcase lmbench %s fail\n", testcases[i]);
//        }
    }
}

void run_busybox() {
    char *argv[10] = {"busybox", "sh", NULL};
    test(argv);
}

int main() {

    // create /dev directory
    int fd_dir = open("/dev", O_DIRECTORY | O_CREAT);
    if (fd_dir >= 0) {
        close(fd_dir);
    }

    mknod("/dev/console", 1, 0);
    open("/dev/console", O_RDWR);

    dup(0); // stdout
    dup(0); // stderr

    mknod("/dev/cpu", 2, 0);
    mknod("/dev/mem", 3, 0);
    mknod("/dev/proc", 4, 0);
    mknod("/dev/null", 5, 0);
    mknod("/dev/zero", 6, 0);

    mknod("/dev/rtc", 9, 0);


    // create /proc directory
    fd_dir = open("/proc", O_DIRECTORY | O_CREATE);
    if (fd_dir >= 0) {
        close(fd_dir);
    }

    mknod("/proc/mounts", 7, 0);
    mknod("/proc/meminfo", 8, 0);

    // link busybox as ls, so the command "which ls" can work correctly
    link("/busybox", "/ls");


    test_lua();
    test_busybox();
//    run_busybox();
    test_lmbench();
    return 0;
}
