#include "fork.h"

extern void __ret_from_fork();
extern int nr_tasks;
extern uint64_t swapper_pg_dir[];
extern uint64_t do_mmap(struct mm_struct *mm, uint64_t addr, uint64_t len, uint64_t vm_pgoff, uint64_t vm_filesz, uint64_t flags);
extern struct task_struct *current;        // 指向当前运行线程的 task_struct
extern struct task_struct *task[NR_TASKS]; // 线程数组，所有的线程都保存在此

//判断虚拟地址 addr 对应的 PTE 是否有效
uint64_t check_pte_validity(uint64_t *pgd, uint64_t va) {
    
    // 第一级页表检查
    if ((pgd[VPN2(va)] & PTE_V) != 0) { 
        pgd = (uint64_t *)((pgd[VPN2(va)] >> 10 << 12) + PA2VA_OFFSET);
    } else {
        return 0;
    }

    // 第二级页表检查
    if ((pgd[VPN1(va)] & PTE_V) != 0) { 
        pgd = (uint64_t *)((pgd[VPN1(va)] >> 10 << 12) + PA2VA_OFFSET); 
    } else {
        return 0;
    }

    // 第三级页表检查
    if ((pgd[VPN0(va)] & PTE_V) != 0) {  
        return 1;  // 如果所有页表项都有效，返回 1
    }else{
        return 0;
    }
    
}

//1. 创建一个新的任务结构体并初始化其信息
struct task_struct* allocate_and_initialize_task() {
    struct task_struct *new_process = (struct task_struct *)kalloc(); // 分配内存
    // 从父进程复制任务结构数据
    memcpy((void *)new_process, (void *)current, PGSIZE);

    // 设置任务状态和其他初始信息
    new_process->state = TASK_RUNNING;
    new_process->pid = nr_tasks;  // 为新进程分配唯一的 PID
    new_process->counter = 0;
    new_process->priority = rand() % (PRIORITY_MAX - PRIORITY_MIN + 1) + PRIORITY_MIN;

    return new_process;
}

// 2. 配置子进程的线程栈信息
void setup_child_thread_stack(struct task_struct *new_process, struct pt_regs *parent_regs) {
    new_process->thread.ra = (uint64_t)__ret_from_fork; // 设置返回地址
    new_process->thread.sepc = parent_regs->sepc;  // 继承父进程的异常程序计数器
    new_process->thread.sp = (uint64_t)new_process + PGSIZE - sizeof(struct pt_regs);  // 设置栈顶
    new_process->thread.sscratch = csr_read(sscratch);  // 获取 sscratch 寄存器值
}

// 3. 为子进程创建页表并初始化
int allocate_and_copy_page_table(struct task_struct *new_process) {
    new_process->pgd = (uint64_t *)alloc_page(); // 分配页表内存
    // 拷贝父进程的页表映射
    memcpy((void *)new_process->pgd, (void *)swapper_pg_dir, PGSIZE);

    return 0;
}

// 4. 复制父进程的虚拟内存区域（VMA）并设置映射
int duplicate_parent_vma(struct task_struct *new_process) {
    for (struct vm_area_struct *current_vma = current->mm.mmap; current_vma; current_vma = current_vma->vm_next) {
        // 为子进程创建新的虚拟内存区域
        if (do_mmap(&new_process->mm, current_vma->vm_start, current_vma->vm_end - current_vma->vm_start,
                    current_vma->vm_pgoff, current_vma->vm_filesz, current_vma->vm_flags) < 0) {
            return -1;
        }
        // 遍历页表
        uint64_t addr = current_vma->vm_start;
        while (addr < current_vma->vm_end) {
            uint64_t physical_addr = check_pte_validity(current->pgd, addr);

            // 如果该页没有有效映射，跳过后续处理
            if (physical_addr) {
                uint64_t child_virtual_addr = (uint64_t)alloc_page();  // 分配新页面

                if (child_virtual_addr) {
                    memcpy((void *)child_virtual_addr, PGROUNDDOWN(addr), PGSIZE);  // 拷贝页面内容
                    create_mapping(new_process->pgd, PGROUNDDOWN(addr), PGROUNDDOWN(child_virtual_addr) - PA2VA_OFFSET, PGSIZE, (current_vma->vm_flags) | PTE_U | PTE_V);
                } else {
                    // 如果分配新页面失败，可以考虑错误处理
                    return -1;
                }
            }

            addr += PGSIZE;  // 更新地址
        }

    }

    return 0;
}


// 5. 配置子进程的寄存器信息
void initialize_child_registers(struct pt_regs *child_regs, struct pt_regs *parent_regs, uint64_t child_sp) {
    memcpy(child_regs, parent_regs, sizeof(struct pt_regs));  // 复制父进程的寄存器状态
    child_regs->x[10] = 0;  // 设置 a0 寄存器为 0，表示子进程的返回值
    child_regs->x[2] = child_sp;  // 设置子进程的栈指针
    child_regs->sepc = parent_regs->sepc + 4;  // 设置子进程的异常程序计数器
}

// 6. 将新进程加入任务调度队列
void schedule_new_task(struct task_struct *new_process) {
    task[nr_tasks++] = new_process;  // 将新任务加入调度队列并更新任务数
    printk(BLUE"[do_fork] [PID: %d] forked from [PID: %d]\n"CLEAR, new_process->pid, current->pid);
}


// 7. 主进程创建函数
uint64_t do_fork(struct pt_regs *regs) {
    bool success = true;

    // 1. 创建并初始化新进程
    struct task_struct *new_process = allocate_and_initialize_task();
    if (!new_process) {
        success = false;  // 创建进程失败
    }

    // 2. 设置子进程的线程栈信息
    if (success) setup_child_thread_stack(new_process, regs);

    // 3. 为子进程分配并复制页表
    if (success && allocate_and_copy_page_table(new_process) < 0) {
        success = false;  // 页表分配失败
    }

    // 4. 复制父进程的 VMA 和映射
    if (success && duplicate_parent_vma(new_process) < 0) {
        success = false;  // VMA 复制失败
    }

    // 5. 初始化子进程的寄存器
    if (success) {
        struct pt_regs *child_regs = (struct pt_regs *)new_process->thread.sp;
        initialize_child_registers(child_regs, regs, new_process->thread.sp);
    }

    // 6. 将新进程加入调度队列
    if (success) schedule_new_task(new_process);

    // 7. 返回新进程的 PID
    return success ? new_process->pid : -1;
}
