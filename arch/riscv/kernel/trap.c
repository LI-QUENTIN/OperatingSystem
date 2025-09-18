#include "stdint.h"
#include "printk.h"
#include "clock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"
#include "string.h"
#include "fork.h"

extern create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, uint64_t perm);
extern char _sramdisk[];
extern struct task_struct* current;

void do_page_fault(struct pt_regs *regs) {
    uint64_t stval = regs->stval; // 获取发生错误的虚拟地址
    uint64_t scause = csr_read(scause); // 获取造成异常的原因

    // 输出调试信息
    printk(YELLOW"[trap.c, do_page_fault] [PID = %d PC = 0x%lx] valid page fault at `0x%lx` with cause %d\n"CLEAR,
           current->pid, regs->sepc, stval, scause);

    // 查找错误地址所对应的 VMA
    struct vm_area_struct *vma = find_vma(&current->mm, stval);

    // 如果地址没有对应的 VMA
    if (!vma) {
        Err("[S] Page Fault: Bad Address 0x%lx\n", stval); // 输出错误信息
        return;
    }

    // 判断是否发生了非法的访问（根据权限判断）
    if ((scause == 0xc && !(vma->vm_flags & VM_EXEC)) ||  
        (scause == 0xd && !(vma->vm_flags & VM_READ)) || 
        (scause == 0xf && !(vma->vm_flags & VM_WRITE))) { 
        Err("[S] Page Fault: Illegal page fault 0x%lx\n", stval);
        return;
    }

    // 合法的页面错误，接下来进行处理
    uint64_t page = alloc_page(); // 分配一个新的物理页

    // 设置映射权限并创建映射
    create_mapping(current->pgd, PGROUNDDOWN(stval), page - PA2VA_OFFSET, PGSIZE, 
    (vma->vm_flags & VM_READ) | (vma->vm_flags & VM_WRITE) | (vma->vm_flags & VM_EXEC) | PTE_U | PTE_V);

    // 匿名映射的页面处理
    if (!(vma->vm_flags & VM_ANON)) { 
        // 非匿名映射，读取 ELF 中的数据
        uint64_t segment_start = (uint64_t)_sramdisk + vma->vm_pgoff; // ELF 段在物理内存中的起始地址
        uint64_t segment_end = (uint64_t)_sramdisk + vma->vm_pgoff + vma->vm_filesz;        // ELF 段在物理内存中的结束地址
        uint64_t page_start = (uint64_t)_sramdisk + vma->vm_pgoff + PGROUNDDOWN(stval) - vma->vm_start; // 错误发生页的起始地址
        uint64_t offset = (PGROUNDDOWN(stval) == PGROUNDDOWN(segment_start)) ? (segment_start & 0xfff) : 0;

        if (segment_end > page_start + PGSIZE) {
            memcpy((void *)(page + offset), (void *)page_start, PGSIZE - offset); 
        } else if (segment_end > page_start && segment_end <= page_start + PGSIZE) {
            memcpy((void *)(page + offset), (void *)page_start, segment_end - page_start - offset);
        }

    } else {
        return;
    }

}

void trap_handler(uint64_t scause, uint64_t sepc, struct pt_regs *regs) {
    uint64_t interrupt_type = scause & 0x7FFFFFFFFFFFFFFF;  // 提取中断类型

    if (scause >> 63) {  // 如果是中断
        switch (interrupt_type) {
            case 5:  // Timer interrupt
                clock_set_next_event();
                do_timer();
                //printk("[S] Supervisor Mode Timer Interrupt\n");
                break;
            case 1:  // Software interrupt
                printk("[S] Supervisor Mode Software Interrupt\n");
                break;
            case 9:  // External interrupt
                printk("[S] Supervisor Mode External Interrupt\n");
                break;
            case 13: // Counter-overflow interrupt
                printk("[S] Counter-overflow Interrupt\n");
                break;
            default:
                printk("Unknown interrupt: %lx\n", interrupt_type);
                break;
        }
    } else {  // 如果是异常
        switch (interrupt_type) {
            case 8:  // Environment call from U-mode (syscall)
                //printk("x[17]: %d\n", regs->x[17]);
                //printk("[S] Environmental call from U-mode\n");
                switch (regs->x[17]) {  // a7
                    case SYS_WRITE:
                        //sys_write(regs->x[10], (const char*)regs->x[11], (size_t)regs->x[12]);  // a0, a1, a2
                        regs->x[10] = sys_write(regs->x[10], (const char *)regs->x[11], regs->x[12]);
                        break;
                    case SYS_READ:
                        regs->x[10] = stdin_read(regs->x[10], (const char *)regs->x[11], regs->x[12]);
                        break;
                    case SYS_GETPID:
                        sys_getpid(regs);
                        break;
                    case SYS_CLONE:
                        regs->x[10]=do_fork(regs);
                    default:
                        break;
                }
                regs->sepc += 4;  // 手动完成 sepc + 4
                break;
            case 2:
                printk("Instruction exception\n");
                break;
            case 3:  // Breakpoint
                printk("Breakpoint\n");
                break;
            case 5:  // Load exception
                printk("Load exception\n");
                break;
            case 7:  // Store/AMO exception
                printk("Store/AMO exception\n");
                break;
            case 9:  // Environment call exception
                printk("Environment call exception\n");
                break;
            case 12:  // Instruction page fault
                printk(RED"Instruction page fault\n"CLEAR);
                do_page_fault(regs);
                break;
            case 13:  // Load page fault
                printk(RED"Load page fault\n"CLEAR);
                do_page_fault(regs);
                break;
            case 15:  // Store/AMO page fault
                printk(RED"Store/AMO page fault\n"CLEAR);
                do_page_fault(regs);
                break;
            case 18:  // Software check
                printk("Software check\n");
                break;
            case 19:  // Hardware error
                printk("Hardware error\n");
                break;
            default:
                printk("Reserved\n");
                break;
        }
    }
}


