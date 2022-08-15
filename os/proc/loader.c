#include <ucore/defs.h>
#include <mem/memory_layout.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <ucore/ucore.h>
#include <fs/fs.h>
static int app_cur, app_num;
static uint64 *app_info_ptr;
extern char _app_num[], _app_names[];
// const uint64 BASE_ADDRESS = 0x1000; // user text start
#define APP_NAME_MAX 100
#define APP_MAX_CNT 50
char names[APP_MAX_CNT][APP_NAME_MAX];

// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
    uint magic;  // must equal ELF_MAGIC
    uchar elf[12];
    ushort type;
    ushort machine;
    uint version;
    uint64 entry;
    uint64 phoff;
    uint64 shoff;
    uint flags;
    ushort ehsize;
    ushort phentsize;
    ushort phnum;
    ushort shentsize;
    ushort shnum;
    ushort shstrndx;
};

// Program section header
struct proghdr {
    uint32 type;
    uint32 flags;
    uint64 off;
    uint64 vaddr;
    uint64 paddr;
    uint64 filesz;
    uint64 memsz;
    uint64 align;
};

// Values for Proghdr type
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7               /* Thread local storage segment */
#define PT_LOOS    0x60000000      /* OS-specific */
#define PT_HIOS    0x6fffffff      /* OS-specific */
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7fffffff
#define PT_GNU_EH_FRAME	(PT_LOOS + 0x474e550)
#define PT_GNU_STACK	(PT_LOOS + 0x474e551)
#define PT_GNU_RELRO	(PT_LOOS + 0x474e552)
#define PT_GNU_PROPERTY	(PT_LOOS + 0x474e553)

// elf file types
#define ET_EXEC   2
#define ET_DYN    3

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4

// for auxv
#define AT_NULL   0	/* end of vector */
#define AT_IGNORE 1	/* entry should be ignored */
#define AT_EXECFD 2	/* file descriptor of program */
#define AT_PHDR   3	/* program headers for program */
#define AT_PHENT  4	/* size of program header entry */
#define AT_PHNUM  5	/* number of program headers */
#define AT_PAGESZ 6	/* system page size */
#define AT_BASE   7	/* base address of interpreter */
#define AT_FLAGS  8	/* flags */
#define AT_ENTRY  9	/* entry point of program */
#define AT_NOTELF 10	/* program is not ELF */
#define AT_UID    11	/* real uid */
#define AT_EUID   12	/* effective uid */
#define AT_GID    13	/* real gid */
#define AT_EGID   14	/* effective gid */
#define AT_PLATFORM 15  /* string identifying CPU for optimizations */
#define AT_HWCAP  16    /* arch dependent hints at CPU capabilities */
#define AT_CLKTCK 17	/* frequency at which times() increments */
/* AT_* values 18 through 22 are reserved */
#define AT_SECURE 23   /* secure mode boolean */
#define AT_BASE_PLATFORM 24	/* string identifying real platform, may
				 * differ from AT_PLATFORM. */
#define AT_RANDOM 25	/* address of 16 random bytes */
#define AT_HWCAP2 26	/* extension of AT_HWCAP */

#define AT_EXECFN  31	/* filename of program */
/* Pointer to the global system page used for system calls and other nice things.  */
#define AT_SYSINFO      32
#define AT_SYSINFO_EHDR 33

#define ADD_AUXV(auxv, t, v) \
    do { \
        (auxv)->type = (t); \
        (auxv)->val = (v); \
        (auxv)++; \
    } while (0)


void init_app_names()
{
    char *s;
    app_info_ptr = (uint64 *)_app_num;
    app_cur = -1;
    app_num = *app_info_ptr;
    app_info_ptr++;

    s = _app_names;
    for (int i = 0; i < app_num; ++i)
    {
        int len = strlen(s);
        strncpy(names[i], (const char *)s, len);
        s += len + 1;
        printf("app name: %s\n", names[i]);
    }
}

int get_app_id_by_name(char *name)
{
    for (int i = 0; i < app_num; ++i)
    {
        if (strncmp(name, names[i], APP_NAME_MAX) == 0)
            return i;
    }
    return -1;
}

void alloc_ustack(struct proc *p)
{
    for (uint64 va = USER_STACK_BOTTOM - USTACK_SIZE; va < USER_STACK_BOTTOM; va += PGSIZE) {
        void *pa = alloc_physical_page();
        if (!pa) {
            panic("alloc_ustack::alloc_physical_page failed");
        }
        if (mappages(p->pagetable, va, PGSIZE, (uint64)pa, PTE_U | PTE_R | PTE_W | PTE_X) != 0) {
            panic("alloc_ustack::mappages failed");
        }
    }
    p->ustack_bottom = USER_STACK_BOTTOM;
    p->trapframe->sp = p->ustack_bottom;
}

void bin_loader(uint64 start, uint64 end, struct proc *p)
{
    debugf("load range = [%p, %p)", start, end);
    uint64 s = PGROUNDDOWN(start), e = PGROUNDUP(end), length = e - s;
    for (uint64 va = USER_TEXT_START, pa = s; pa < e; va += PGSIZE, pa += PGSIZE)
    {
        void *page = alloc_physical_page();
        if (page == NULL)
        {
            panic("bin_loader alloc_physical_page");
        }
        memmove(page, (const void *)pa, PGSIZE);
        if (mappages(p->pagetable, va, PGSIZE, (uint64)page, PTE_U | PTE_R | PTE_W | PTE_X) != 0)
            panic("bin_loader mappages");
    }

    p->trapframe->epc = USER_TEXT_START;
    alloc_ustack(p);
    p->next_shmem_addr = (void*) p->ustack_bottom+PGSIZE;
    p->total_size = USTACK_SIZE + length;
    p->heap_start = USER_TEXT_START + length;
}

void loader(int id, struct proc *p) {
    infof("loader %s", names[id]);
    bin_loader(app_info_ptr[id], app_info_ptr[id + 1], p);
}

static void print_buf(char *buf, uint64 len, uint64 va)
{
//    for (uint64 i = 0; i < len; ++i)
//    {
//        printf("%x", buf[i] >> 4);
//        printf("%x", buf[i] & 0xf);
//        printf(" ");
//        if ((i + 1) % 16 == 0) {
//            printf("\n");
//        }
//    }
//    printf("\n");
    const int line_len = 16;
    uint64 skip = va % line_len;
    uint64 vlen = len + skip;
    uint64 vva = va - skip;
    for (uint64 i = 0; i < vlen; i += line_len)
    {
        printf("%p: ", vva + i);
        for (uint64 j = 0; j < line_len; ++j)
        {
            if (i + j < va % line_len) {
                printf("?? ");
            } else {
                printf("%x", buf[i + j - skip] >> 4);
                printf("%x", buf[i + j - skip] & 0xf);
                printf(" ");
            }
        }
        printf("\n");
    }
    printf("\n");
}

static int
loadseg(pagetable_t pagetable, uint64 va, struct inode *inode, uint offset, uint sz)
{
    uint64 i;
    uint64 pa_align, pa;
    uint64 va_align, va_begin, va_end, copy_size, temp_size = 0;

    // prefix part
    va_align = PGROUNDDOWN(va);
    va_begin = va;
    va_end = MIN(PGROUNDUP(va), va_begin + sz);
    if (va_begin != va_end) {
        pa_align = walkaddr(pagetable, va_align);
        if (pa_align == NULL) {
            panic("loadseg: address should exist");
        }
        pa = pa_align + (va_begin - va_align);
        copy_size = va_end - va_begin;
//        infof("loadseg: copy prefix part: va = %p, pa = %p, size = %p", va_begin, pa, copy_size);
        if(readi(inode, 0, (void*)pa, offset + temp_size, copy_size) != copy_size) {
            infof("loadseg: readi error, read offset: %x, read size: %x", offset + temp_size, copy_size);
            return -1;
        }
//        print_buf((char*)pa, copy_size, va_begin);
        temp_size += copy_size;
    }
    if (temp_size == sz) {
        return 0;
    }

    // middle part and suffix part
    va_begin = PGROUNDUP(va);
    va_end = va + sz;
    for (i = va_begin; i < va_end; i += PGSIZE) {
        pa = walkaddr(pagetable, i);
        if (pa == NULL) {
            panic("loadseg: address should exist");
        }
        copy_size = MIN(PGSIZE, va_end - i);
//        infof("loadseg: copy middle part: va = %p, pa = %p, size = %p", i, pa, copy_size);
        if(readi(inode, 0, (void*)pa, offset + temp_size, copy_size) != copy_size) {
            infof("loadseg: readi error, read offset: %x, read size: %x", offset + temp_size);
            return -1;
        }
//        print_buf((char*)pa, copy_size, i);
        temp_size += copy_size;
    }

    return 0;
}

//static int
//cleanseg(pagetable_t pagetable, uint64 va, uint sz)
//{
//    uint64 i;
//    uint64 pa_align, pa;
//    uint64 va_align, va_begin, va_end, copy_size, temp_size = 0;
//
//    // prefix part
//    va_align = PGROUNDDOWN(va);
//    va_begin = va;
//    va_end = MIN(PGROUNDUP(va), va_begin + sz);
//    if (va_begin != va_end) {
//        pa_align = walkaddr(pagetable, va_align);
//        if (pa_align == NULL) {
//            panic("cleanseg: address should exist");
//        }
//        pa = pa_align + (va_begin - va_align);
//        copy_size = va_end - va_begin;
//        infof("cleanseg: clean prefix part: va = %p, pa = %p, size = %p", va_begin, pa, copy_size);
//        memset((void*)pa, 0, copy_size);
//        temp_size += copy_size;
//    }
//    if (temp_size == sz) {
//        return 0;
//    }
//
//    // middle part and suffix part
//    va_begin = PGROUNDUP(va);
//    va_end = va + sz;
//    for (i = va_begin; i < va_end; i += PGSIZE) {
//        pa = walkaddr(pagetable, i);
//        if (pa == NULL) {
//            panic("cleanseg: address should exist");
//        }
//        copy_size = MIN(PGSIZE, va_end - i);
//        infof("cleanseg: clean middle part: va = %p, pa = %p, size = %p", i, pa, copy_size);
//        memset((void*)pa, 0, copy_size);
//        temp_size += copy_size;
//    }
//
//    return 0;
//}

static void print_proghdr(struct proghdr *ph)
{
    printf("type: %x\n", ph->type);
    printf("flags: %x\n", ph->flags);
    printf("off: %x\n", ph->off);
    printf("vaddr: %x\n", ph->vaddr);
    printf("paddr: %x\n", ph->paddr);
    printf("filesz: %x\n", ph->filesz);
    printf("memsz: %x\n", ph->memsz);
    printf("align: %x\n", ph->align);
    printf("\n");
}

int loadelf(struct proc *p, struct inode *ip, bool is_interp, struct elfhdr
        *ehdr,
        uint64 *base, uint *npages) {

    // Check ELF header
    if (readi(ip, FALSE, ehdr, 0, sizeof(struct elfhdr)) != sizeof(struct elfhdr)) {
        infof("elf_loader read elf header failed");
        return -1;
    }

    bool is_dyn = ehdr->type == ET_DYN;

    // calculate load range
    uint64 min_va = 0xffffffffffffffff;
    uint64 max_va = 0;
    struct proghdr phdr;
    for (uint64 i = 0; i < ehdr->phnum; i++) {
        if (readi(ip, FALSE, &phdr, ehdr->phoff + i * sizeof(struct proghdr), sizeof(struct proghdr)) != sizeof(struct proghdr)) {
            infof("elf_loader read program header failed");
            return -1;
        }
        if (phdr.type != PT_LOAD) {
            continue;
        }
        min_va = MIN(min_va, phdr.vaddr);
        max_va = MAX(max_va, phdr.vaddr + phdr.memsz);
    }
    min_va = PGROUNDDOWN(min_va);
    max_va = PGROUNDUP(max_va);
    infof("elf_loader min_va: %p, max_va: %p, len: %d", min_va, max_va, max_va - min_va);

    // allocate memory
    uint64 map_base = 0;
    if (!is_interp) {
        if (is_dyn) {
            map_base = USER_TEXT_START;
            if (uvmalloc(p->pagetable, USER_TEXT_START, USER_TEXT_START +
            max_va) != USER_TEXT_START + max_va) {
                infof("elf_loader uvmalloc failed");
                return -1;
            }
        } else {
            map_base = min_va;
            if (uvmalloc(p->pagetable, USER_TEXT_START, max_va) != max_va) {
                infof("elf_loader uvmalloc failed");
                return -1;
            }
        }

    } else {
        map_base = (uint64)mmap(p, NULL, max_va - min_va, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, NULL, 0);
        if ((void *)map_base == MAP_FAILED) {
            infof("elf_loader mmap failed");
            return -1;
        }
    }

    // load segments into memory
    for (uint64 i = 0; i < ehdr->phnum; i++) {
        if (readi(ip, FALSE, &phdr, ehdr->phoff + i * sizeof(struct proghdr), sizeof(struct proghdr)) != sizeof(struct proghdr)) {
            infof("elf_loader read program header failed");
            return -1;
        }
        if (phdr.type != PT_LOAD) {
            continue;
        }
        uint64 loadva = phdr.vaddr + (map_base - min_va);
        if (loadseg(p->pagetable, loadva, ip, phdr.off, phdr.filesz) < 0) {
            infof("elf_loader loadseg failed");
            return -1;
        }
    }

    // fill output info
    *base = map_base;
    *npages = (max_va - min_va) / PGSIZE;
    return 0;
}

int get_interp(struct inode *ip, char *path) {

    // read elf header
    struct elfhdr ehdr;
    if (readi(ip, FALSE, &ehdr, 0, sizeof(struct elfhdr)) != sizeof(struct elfhdr)) {
        infof("elf_loader read elf header failed");
        return -1;
    }

    // try to find interpreter
    struct proghdr phdr;
    for (uint64 i = 0; i < ehdr.phnum; i++) {
        if (readi(ip, FALSE, &phdr, ehdr.phoff + i * sizeof(struct proghdr), sizeof(struct proghdr)) != sizeof(struct proghdr)) {
            infof("elf_loader read program header failed");
            return -1;
        }
        if (phdr.type != PT_INTERP) {
            continue;
        }
        if (readi(ip, FALSE, path, phdr.off, phdr.filesz) != phdr.filesz) {
            infof("elf_loader read interpreter path failed");
            return -1;
        }
        path[phdr.filesz] = '\0';
        return 0;
    }

    // interpreter not found
    return -1;
}

static int check_elf_header(struct inode *ip) {
    struct elfhdr ehdr;
    if (readi(ip, FALSE, &ehdr, 0, sizeof(struct elfhdr)) != sizeof(struct elfhdr)) {
        infof("elf_loader read elf header failed");
        return -1;
    }
    if (ehdr.magic != ELF_MAGIC) {
        infof("elf_loader invalid elf magic");
        return -1;
    }
    if (ehdr.phnum == 0) {
        infof("elf_loader no program header");
        return -1;
    }
    return 0;
}

int check_script_header(char *name, char *interp, char *opt_arg) {
    struct inode* ip = inode_by_name(name);
    if (ip == NULL) {
        infof("elf_loader inode_by_name exec failed");
        return -1;
    }

    ilock(ip);

    char line[MAXPATH];
    int length;
    if ((length = readi(ip, FALSE, line, 0, MAXPATH - 1)) < 2) {
        iunlockput(ip);
        infof("not a valid script (1)");
        return -1;
    }
    line[length - 1] = '\0';
    if (line[0] != '#' || line[1] != '!') {
        iunlockput(ip);
        infof("not a valid script (2)");
        return -1;
    }

    // write interpreter to caller's variable
    char *_interp = line + 2;
    while (*_interp == ' ') {
        _interp++;
    }
    char *_interp_end = _interp;
    while (*_interp_end != ' ' && *_interp_end != '\n' && *_interp_end != '\0') {
        _interp_end++;
    }
    *_interp_end = '\0';
    strcpy(interp, _interp);

    // write optional arg to caller's variable
    char *_opt_arg = _interp_end + 1;
    while (*_opt_arg == ' ') {
        _opt_arg++;
    }
    char *_opt_arg_end = _opt_arg;
    while (*_opt_arg_end != ' ' && *_opt_arg_end != '\n' && *_opt_arg_end != '\0') {
        _opt_arg_end++;
    }
    *_opt_arg_end = '\0';
    strcpy(opt_arg, _opt_arg);

    iunlockput(ip);
    return 0;
}

int elf_loader(char* name, struct proc *p, struct auxv_t *auxv, int *auxc) {
    infof("elf_loader %s", name);

    struct inode* ip = inode_by_name(name);
    if (ip == NULL) {
        infof("elf_loader inode_by_name exec failed");
        return -1;
    }

    ilock(ip);

    // check elf header
    if (check_elf_header(ip) < 0) {
        iunlockput(ip);
        infof("elf_loader not a elf");
        return -1;
    }

    proc_free_mem_and_pagetable(p);
    p->total_size = 0;
    p->pagetable = proc_pagetable(p);
    KERNEL_ASSERT(p->pagetable != NULL, "elf_loader alloc page table failed");

    // can't revoke the modification from here because the old page table is freed,
    // so we must use panic.

    struct elfhdr ehdr[2];
    uint64 base[2];
    uint npages[2];

    // load exec
    if (loadelf(p, ip, FALSE, &ehdr[0], &base[0], &npages[0]) < 0) {
        panic("elf_loader loadelf exec failed");
    }

    // find interpreter and load it if exists
    bool has_interp = FALSE;
    char interp_path[MAXPATH];
    if (get_interp(ip, interp_path) == 0) {
        has_interp = TRUE;
        struct inode* interp_ip = inode_by_name(interp_path);
        if (interp_ip == NULL) {
            panic("elf_loader inode_by_name dyn failed");
        }
        ilock(interp_ip);
        if (loadelf(p, interp_ip, TRUE, &ehdr[1], &base[1], &npages[1]) < 0) {
            panic("elf_loader loadelf dyn failed");
        }
        iunlockput(interp_ip);
    }

    iunlockput(ip);

    uint64 bin_entry =ehdr[0].type == ET_EXEC ? ehdr[0].entry : base[0] + ehdr[0].entry;

    // fill auxv info
    struct auxv_t *auxv_base = auxv;
    ADD_AUXV(auxv, AT_PHDR, base[0] + ehdr[0].phoff);
    ADD_AUXV(auxv, AT_PHENT, sizeof(struct proghdr));
    ADD_AUXV(auxv, AT_PHNUM, ehdr[0].phnum);
    ADD_AUXV(auxv, AT_PAGESZ, PGSIZE);
    ADD_AUXV(auxv, AT_ENTRY, bin_entry);
    if (has_interp) {
        ADD_AUXV(auxv, AT_BASE, base[1]); // record interpreter base
    }
    ADD_AUXV(auxv, AT_RANDOM, USER_STACK_BOTTOM - RANDOM_SIZE);
    *auxc = auxv - auxv_base;
    infof("elf_loader auxc %d", *auxc);
    for (int i = 0; i < *auxc; i++) {
        infof("elf_loader auxv[%d] %x:%p", i, auxv_base[i].type, auxv_base[i].val);
    }
    uint64 entry = has_interp ? base[1] + ehdr[1].entry : bin_entry;
    p->trapframe->epc = entry;
    infof("elf_loader epc %p", entry);
    alloc_ustack(p);
    p->next_shmem_addr = (void*) p->ustack_bottom+PGSIZE;
    uint64 edata = base[0] + npages[0] * PGSIZE;
    p->total_size = USTACK_SIZE + (edata - USER_TEXT_START);
    p->heap_start = edata;
    infof("elf_loader total_size %p", p->total_size);
    return 0;
}

// load shell from kernel data section
// and make it a proc
int make_shell_proc()
{
    struct proc *p = alloc_proc();

    // still need to init: 
    //  * parent           
    //  * ustack_bottom    
    //  * total_size       
    //  * cwd              
    //  * name
    //  * next_shmem_addr             

    // parent
    p->parent = NULL;

    int id = get_app_id_by_name( "test_runner" );
    if (id < 0)
        panic("no user shell");
    loader(id, p);  // will fill ustack_bottom, next_shmem_addr and total_size

    // name
    safestrcpy(p->name, "shell", PROC_NAME_MAX);

    // cwd
//    p->cwd = inode_by_name("/");
    p->cwd = NULL;
    p->state = RUNNABLE;
    release(&p->lock);

    return 0;
}
