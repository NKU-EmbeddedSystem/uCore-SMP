#include <proc/proc.h>
#include <trap/trap.h>
#include <utils/log.h>

#define AUX_CNT 38

static void debug_print_args(char *name, int argc, const char **argv) {
    tracecore("exec name=%s, argc=%d", name, argc);
    for (int i = 0; i < argc; i++) {
        tracecore("argv[%d]=%s", i, argv[i]);
    }
}

static char* round_down_8 (char* p){
    return (char*)(((uint64)p) & (~0x7ULL));
}

int exec(char *name, int argc, const char **argv, int envc, const char **envp) {
    // fix name cwd at first
    name = fix_cwd_slashes(name);

    // show exec params
    debug_print_args(name, argc, argv);
    
//    int id = get_app_id_by_name(name);
//    if (id < 0)
//        return -1;
    struct proc *p = curr_proc();

//    move to elf loader
//    proc_free_mem_and_pagetable(p);
//    p->total_size = 0;
//    p->pagetable = proc_pagetable(p);
//    if (p->pagetable == 0) {
//        panic("");
//    }

//    loader(id, p);
    struct auxv_t auxv[AUX_CNT];
    int auxc;

    // put these params on stack is ok
    // because it will recursive call exec, so the current stack is not freed
    char interp[MAXPATH] = {0};
    char opt_arg[MAXPATH] = {0};

    if (elf_loader(name, p, auxv, &auxc) >= 0) {
        goto load_success;
    } else if (check_script_header(name, interp, opt_arg) >= 0) {
        // interpreter [optional-arg] pathname arg...
        // arg...  is the series of words pointed to by the argv argument of execve(),
        // starting at argv[1].

        bool has_opt_arg = (opt_arg[0] != '\0');
        // move the argument
        int move = has_opt_arg ? 2 : 1;
        for (int i = argc - 1; i >= 0; i--) {
            argv[i + move] = argv[i];
        }
        // fill interp, opt_arg
        int i = 0;
        argv[i++] = interp;
        if (has_opt_arg) {
            argv[i++] = opt_arg;
        }
        // overwrite original argv[0]
        argv[i++] = name;
        // adjust argc
        argc += move;

        // recursive call to exec
        return exec(interp, argc, argv, envc, envp);
    } else {
        return -1;
    }

load_success:
    safestrcpy(p->name, name, PROC_NAME_MAX);
    // push args
    char *sp = (char *)p->trapframe->sp;
    phex(sp);

    // sp itself is on the boundary hence not mapped, but sp-1 is a valid address.
    // we can calculate the physical address of sp
    // but can NOT access sp_pa
    char *sp_pa = (char *)(virt_addr_to_physical(p->pagetable, (uint64)sp - 1) + 1);

    char *sp_pa_bottom = sp_pa; // keep a record

    // the argv array (content of argv[i]) will be stored on ustack, at the very bottom
    // and the real string value of argv array (content of *argv[i]) will be stored after that

    // copy argv to user stack
    for (int i = 0; i < argc; i++) {
        int arg_len = strlen(argv[i]);
        sp_pa -= arg_len + 1; // alloc space for argv[i] string, including a null
        sp_pa = round_down_8(sp_pa); // round down to multiple of 4
        strncpy(sp_pa, argv[i], arg_len);
        sp_pa[arg_len] = '\0';                       // make sure null is there
        argv[i] = (sp_pa - sp_pa_bottom) + sp; // point argv[i] to here, use user space
    }

    // copy envp to user stack
    for (int i = 0; i < envc; i++) {
        int arg_len = strlen(envp[i]);
        sp_pa -= arg_len + 1; // alloc space for envp[i] string, including a null
        sp_pa = round_down_8(sp_pa); // round down to multiple of 4
        strncpy(sp_pa, envp[i], arg_len);
        sp_pa[arg_len] = '\0';                       // make sure null is there
        envp[i] = (sp_pa - sp_pa_bottom) + sp; // point envp[i] to here, use user space
    }

//    // write 16 random bytes to stack bottom
    sp_pa -= RANDOM_SIZE;
    for (int i = 0; i < RANDOM_SIZE; i++) {
        sp_pa[i] = r_cycle() & 0xff;
    }

    // copy argv and envp pointer array to user stack
    sp_pa -= (argc + envc + 2 + (auxc + 1) * 2) * sizeof(const char *);           // alloc space for argv, envp and auxv, the last ptr is NULL
    const char **argv_start = (const char **)sp_pa; // user main()'s argv value (physical here)
    const char **envp_start = (const char **)sp_pa + argc + 1;
    struct auxv_t * const auxv_start = (struct auxv_t * const)((const char **)sp_pa + argc + envc + 2);
    for (int i = 0; i < argc; i++) {
        argv_start[i] = argv[i];
    }
    argv_start[argc] = NULL;

    for (int i = 0; i < envc; i++) {
        envp_start[i] = envp[i];
    }
    envp_start[envc] = NULL;

    // auxv
    for (int i = 0; i < auxc; i++) {
        auxv_start[i] = auxv[i];
    }
    auxv_start[auxc] = (struct auxv_t){0, 0};

    // construct argc in stack
    // notice: according to "_start" entry, this is not 8 byte aligned
    sp_pa -= sizeof(long);
    *(long*)sp_pa = argc;

    p->trapframe->sp += sp_pa - sp_pa_bottom; // update user sp

    // because glibc use this parameter to do exit,
    // so we need to set it to 0
    p->trapframe->a0 = 0;

    return 0;
}
