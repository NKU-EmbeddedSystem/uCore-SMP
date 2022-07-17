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
#define ELF_PROG_LOAD           1
#define ELF_PROG_GNU_STACK      0x6474E551
#define ELF_PROG_GNU_RELRO      0x6474E552

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4


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
        infof("loadseg: copy prefix part: va = %p, pa = %p, size = %p", va_begin, pa, copy_size);
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
        infof("loadseg: copy middle part: va = %p, pa = %p, size = %p", i, pa, copy_size);
        if(readi(inode, 0, (void*)pa, offset + temp_size, copy_size) != copy_size) {
            infof("loadseg: readi error, read offset: %x, read size: %x", offset + temp_size);
            return -1;
        }
//        print_buf((char*)pa, copy_size, i);
        temp_size += copy_size;
    }

    return 0;
}

static int
cleanseg(pagetable_t pagetable, uint64 va, uint sz)
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
            panic("cleanseg: address should exist");
        }
        pa = pa_align + (va_begin - va_align);
        copy_size = va_end - va_begin;
        infof("cleanseg: clean prefix part: va = %p, pa = %p, size = %p", va_begin, pa, copy_size);
        memset((void*)pa, 0, copy_size);
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
            panic("cleanseg: address should exist");
        }
        copy_size = MIN(PGSIZE, va_end - i);
        infof("cleanseg: clean middle part: va = %p, pa = %p, size = %p", i, pa, copy_size);
        memset((void*)pa, 0, copy_size);
        temp_size += copy_size;
    }

    return 0;
}

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

int elf_loader(char* name, struct proc *p) {
    infof("elf_loader %s", name);

    struct inode* inode = inode_by_name(name);
    if (inode == NULL) {
        infof("elf_loader inode_by_name failed");
        return -1;
    }

    ilock(inode);

    struct elfhdr elf;
    struct proghdr ph;
    int i, off;
    uint64 pos = USER_TEXT_START;

    // Check ELF header
    if (readi(inode, FALSE, &elf, 0, sizeof(elf)) != sizeof(elf)) {
        infof("elf_loader read elf header failed");
        iunlockput(inode);
        return -1;
    }
    if (elf.magic != ELF_MAGIC) {
        infof("elf_loader invalid elf magic");
        iunlockput(inode);
        return -1;
    }
    if (elf.phnum == 0) {
        infof("elf_loader no program header");
        iunlockput(inode);
        return -1;
    }

    proc_free_mem_and_pagetable(p);
    p->total_size = 0;
    p->pagetable = proc_pagetable(p);
    KERNEL_ASSERT(p->pagetable != NULL, "elf_loader alloc page table failed");

    // can't revoke the modification from here because the old page table is freed,
    // so we must use panic.

    for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
        if(readi(inode, FALSE, &ph, off, sizeof(ph)) != sizeof(ph)) {
            panic("elf_loader loop readi");
        }
//        print_proghdr(&ph);
        if (ph.type != ELF_PROG_LOAD) {
            continue;
        }
        if (ph.memsz < ph.filesz) {
            panic("elf_loader loop memsz 1");
        }
        if (ph.vaddr + ph.memsz < ph.vaddr) {
            panic("elf_loader loop memsz 2");
        }
        if ((pos = uvmalloc(p->pagetable, pos, ph.vaddr + ph.memsz)) == 0)
            panic("elf_loader loop uvmalloc");
        infof("elf_loader loop vaddr range %p-%p", ph.vaddr, ph.vaddr + ph.memsz);
        if (loadseg(p->pagetable, ph.vaddr, inode, ph.off, ph.filesz) < 0)
            panic("elf_loader loop loadseg");
//        if (ph.memsz - ph.filesz > 0 &&
//            cleanseg(p->pagetable, ph.vaddr + ph.filesz, ph.memsz - ph.filesz) < 0)
//            panic("elf_loader loop cleanseg");
    }

    iunlockput(inode);

    p->trapframe->epc = elf.entry;
    infof("elf_loader epc %p\n", elf.entry);
    alloc_ustack(p);
    p->next_shmem_addr = (void*) p->ustack_bottom+PGSIZE;
    p->total_size = USTACK_SIZE + (pos - USER_TEXT_START);
    p->heap_start = pos;
    infof("elf_loader total_size %p\n", p->total_size);
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

    int id = get_app_id_by_name( "shell" );
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
