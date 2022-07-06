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
    if (mappages(p->pagetable, USER_STACK_BOTTOM - USTACK_SIZE, USTACK_SIZE, (uint64)alloc_physical_page(), PTE_U | PTE_R | PTE_W | PTE_X) != 0)
    {
        panic("alloc_ustack");
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
}

void loader(int id, struct proc *p) {
    infof("loader %s", names[id]);
    bin_loader(app_info_ptr[id], app_info_ptr[id + 1], p);
}

static int
loadseg(pagetable_t pagetable, uint64 va, struct inode *inode, uint offset, uint sz)
{
    uint i, n;
    uint64 pa;
    if((va % PGSIZE) != 0)
        panic("loadseg: va must be page aligned");

    for(i = 0; i < sz; i += PGSIZE){
        pa = walkaddr(pagetable, va + i);
        if(pa == NULL)
            panic("loadseg: address should exist");
        if(sz - i < PGSIZE)
            n = sz - i;
        else
            n = PGSIZE;
        if(readi(inode, 0, (void*)pa, offset+i, n) != n)
            return -1;
    }

    return 0;
}

int elf_loader(char* name, struct proc *p) {
    infof("elf_loader %s", name);

    struct inode* inode = inode_by_name(name);
    if (inode == NULL) {
        infof("elf_loader inode_by_name failed");
        return -1;
    }

    ilock(inode);

    proc_free_mem_and_pagetable(p);
    p->total_size = 0;
    p->pagetable = proc_pagetable(p);
    if (p->pagetable == 0) {
        panic("");
    }

    struct elfhdr elf;
    struct proghdr ph;
    int i, off;
    uint64 oldsz = USER_TEXT_START;

    // Check ELF header
    if (readi(inode, FALSE, &elf, 0, sizeof(elf)) != sizeof(elf)) {
        panic("elf_loader readi");
    }
    if (elf.magic != ELF_MAGIC) {
        panic("elf_loader magic");
    }
    if (elf.phnum == 0) {
        panic("elf_loader phnum");
    }

    for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
        if(readi(inode, FALSE, &ph, off, sizeof(ph)) != sizeof(ph))
            panic("elf_loader loop readi");
        if(ph.type != ELF_PROG_LOAD)
            continue;
        if(ph.memsz < ph.filesz)
            panic("elf_loader loop memsz 1");
        if(ph.vaddr + ph.memsz < ph.vaddr)
            panic("elf_loader loop memsz 2");
        uint64 newsz;
        if((newsz = uvmalloc(p->pagetable, oldsz, ph.vaddr + ph.memsz)) == 0)
            panic("elf_loader loop uvmalloc");
        oldsz = PGROUNDUP(newsz);
        if(ph.vaddr % PGSIZE != 0)
            panic("elf_loader loop vaddr");
        printf("elf_loader loop vaddr %p\n", ph.vaddr);
        if(loadseg(p->pagetable, ph.vaddr, inode, ph.off, ph.filesz) < 0)
            panic("elf_loader loop loadseg");
    }

    iunlockput(inode);

    p->trapframe->epc = elf.entry;
    printf("elf_loader epc %p\n", elf.entry);
    alloc_ustack(p);
    p->next_shmem_addr = (void*) p->ustack_bottom+PGSIZE;
    p->total_size = USTACK_SIZE + (oldsz - USER_TEXT_START);
    printf("elf_loader total_size %p\n", p->total_size);

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
