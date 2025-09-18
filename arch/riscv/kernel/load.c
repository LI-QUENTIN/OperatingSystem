#include "mm.h"
#include "defs.h"
#include "proc.h"
#include "stdlib.h"
#include "string.h"


extern char _sramdisk[];
extern char _eramdisk[];
extern uint64_t swapper_pg_dir[];
extern create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, int perm);

void create_user_stack(struct task_struct *task){
    // 分配用户栈空间
    void *user_stack = alloc_page();

    // 映射用户栈到进程的页表
    create_mapping(task->pgd, USER_END - PGSIZE, (uint64_t)user_stack - PA2VA_OFFSET, PGSIZE, PTE_R | PTE_W | PTE_U | PTE_V);
}

void setup_task_status(struct task_struct *task) {
    // 配置 sstatus 中的 SPP（使得 sret 返回至 U-Mode）， SPIE （sret 之后开启中断）， SUM（S-Mode 可以访问 User 页面）
    // spp = 0 
    task->thread.sstatus &= ~(1 << 8);
    //spie = 1
    task->thread.sstatus |= (1 << 5);
    //sum = 1
    task->thread.sstatus |= (1 << 18);

    // 设置 sscratch 为 U-Mode 的 sp，值为 USER_END
    task->thread.sscratch = USER_END;
}

void init_user_stack(struct task_struct *task) {
    // 为任务分配页表
    task->pgd = (uint64_t *)alloc_page();

    // 将内核页表 (swapper_pg_dir) 复制到任务的页表中
    memcpy(task->pgd, swapper_pg_dir, PGSIZE);

    // 分配用户栈空间
    //create_user_stack(task);
}

void load_program(struct task_struct *task) {
    // 初始化任务的页表并映射用户栈
    init_user_stack(task);

    // 处理 ELF 文件
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)_sramdisk;
    Elf64_Phdr *phdrs = (Elf64_Phdr *)(_sramdisk + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        Elf64_Phdr *phdr = phdrs + i;
        if (phdr->p_type == PT_LOAD) {
            // 计算空间的各个属性
            uint64_t start = PGROUNDDOWN(phdr->p_vaddr);
            uint64_t end = PGROUNDUP(phdr->p_vaddr + phdr->p_memsz);
            uint64_t page_num = (end - start) / PGSIZE;
            uint64_t offset = (phdr->p_vaddr)&0xfff;
            
            //为段分配物理页
            void *mem = alloc_pages(page_num);

            //拷贝段内容
            memcpy((mem + offset), (_sramdisk + phdr->p_offset), phdr->p_filesz);

            //映射
            create_mapping((uint64_t *)task->pgd, start, ((uint64_t)mem - PA2VA_OFFSET), phdr->p_memsz, ((uint64_t)phdr->p_flags << 1 | PTE_V | PTE_U));
        }
    }
    //设置基础的 sstatus 和 sscratch
    setup_task_status(task);
    //user stack
    //设置程序入口地址
    task->thread.sepc = ehdr->e_entry;
}

void load_binary(struct task_struct *task) {
    // 设置基础的 sstatus 和 sscratch
    setup_task_status(task);
    // 复制内核页表到任务页表
    task->pgd = (uint64_t *)alloc_page();
    memcpy(task->pgd, swapper_pg_dir, PGSIZE);

    // 复制应用程序到用户空间
    uint64_t uapp_size = (uint64_t)_eramdisk - (uint64_t)_sramdisk;
    char *uapp = (char *)alloc_pages((uapp_size + PGSIZE - 1) / PGSIZE);
    memcpy(uapp, _sramdisk, uapp_size);

    // 映射应用程序到进程的页表
    create_mapping(task->pgd, USER_START, (uint64_t)uapp - PA2VA_OFFSET, uapp_size, PTE_R | PTE_W | PTE_X | PTE_U | PTE_V);

    // 分配并映射用户栈
    create_user_stack(task);
}

void load_program_map(struct task_struct *task) {
    init_user_stack(task);
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)_sramdisk;
    Elf64_Phdr *phdrs = (Elf64_Phdr *)(_sramdisk + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        Elf64_Phdr *phdr = phdrs + i;
        if (phdr->p_type == PT_LOAD) {
            do_mmap(&task->mm, phdr->p_vaddr, phdr->p_memsz, phdr->p_offset, phdr->p_filesz, (phdr->p_flags & (PF_R | PF_W | PF_X)) ? ( (phdr->p_flags & PF_R ? VM_READ : 0) | (phdr->p_flags & PF_W ? VM_WRITE : 0) | (phdr->p_flags & PF_X ? VM_EXEC : 0)) : 0);
        }
    }
    // user stack
    do_mmap(&task->mm,USER_END-PGSIZE,PGSIZE,0,0,VM_READ|VM_WRITE|VM_ANON);
    task->thread.sepc = ehdr->e_entry;
    setup_task_status(task);
}
