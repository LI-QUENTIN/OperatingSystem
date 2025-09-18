#include "mm.h"
#include "defs.h"
#include "proc.h"
#include "stdlib.h"
#include "printk.h"
#include "load.h"
#include "fs.h"
#include "stddef.h"
#include "virtio.h"
#include "mbr.h"

extern void __dummy();

struct task_struct *idle;           // idle process
struct task_struct *current;        // 指向当前运行线程的 task_struct
struct task_struct *task[NR_TASKS]; // 线程数组，所有的线程都保存在此

extern char _sramdisk[];
extern char _eramdisk[];
extern uint64_t swapper_pg_dir[];

int nr_tasks=2;


void task_init() {
    srand(2024);

    // 1. 调用 kalloc() 为 idle 分配一个物理页
    idle = (struct task_struct*)kalloc();
    // 2. 设置 state 为 TASK_RUNNING;
    idle->state = TASK_RUNNING;
    // 3. 由于 idle 不参与调度，可以将其 counter / priority 设置为 0
    idle->counter = 0;
    idle->priority = 0;
    // 4. 设置 idle 的 pid 为 0
    idle->pid = 0;
    // 5. 将 current 和 task[0] 指向 idle
    current = idle;
    task[0] = idle;

    /* YOUR CODE HERE */

    // 1. 参考 idle 的设置，为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, 此外，counter 和 priority 进行如下赋值：
    //     - counter  = 0;
    //     - priority = rand() 产生的随机数（控制范围在 [PRIORITY_MIN, PRIORITY_MAX] 之间）
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 thread_struct 中的 ra 和 sp
    //     - ra 设置为 __dummy（见 4.2.2）的地址
    //     - sp 设置为该线程申请的物理页的高地址

    /* YOUR CODE HERE */
    for (int i = 1; i < nr_tasks; i++) {
        task[i] = (struct task_struct*)kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = (uint64_t)rand() % (PRIORITY_MAX - PRIORITY_MIN + 1) + PRIORITY_MIN;
        task[i]->pid = i;
        task[i]->thread.ra = (uint64_t)__dummy;
        task[i]->thread.sp = (uint64_t)(task[i]) + PGSIZE;

        task[i]->mm.mmap=NULL;

        task[i]->pgd = (uint64_t *)alloc_page();

        memcpy((void *)task[i]->pgd,(void *)swapper_pg_dir,PGSIZE);

        load_program_map(task[i]);

        task[i]->files = file_init();
    }


    printk("...task_init done!\n");

    virtio_dev_init();
    printk("...virtio_dev_init done!\n");

    mbr_init();
    printk("...mbr_init done!\n");
}

#if TEST_SCHED
#define MAX_OUTPUT ((NR_TASKS - 1) * 10)
char tasks_output[MAX_OUTPUT];
int tasks_output_index = 0;
char expected_output[] = "2222222222111111133334222222222211111113";
#include "sbi.h"
#endif

void dummy() {
    uint64_t MOD = 1000000007;
    uint64_t auto_inc_local_var = 0;
    int last_counter = -1;
    while (1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            if (current->counter == 1) {
                --(current->counter);   // forced the counter to be zero if this thread is going to be scheduled
            }                           // in case that the new counter is also 1, leading the information not printed.
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
            #if TEST_SCHED
            tasks_output[tasks_output_index++] = current->pid + '0';
            if (tasks_output_index == MAX_OUTPUT) {
                for (int i = 0; i < MAX_OUTPUT; ++i) {
                    if (tasks_output[i] != expected_output[i]) {
                        printk("\033[31mTest failed!\033[0m\n");
                        printk("\033[31m    Expected: %s\033[0m\n", expected_output);
                        printk("\033[31m    Got:      %s\033[0m\n", tasks_output);
                        sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN, SBI_SRST_RESET_REASON_NONE);
                    }
                }
                printk("\033[32mTest passed!\033[0m\n");
                printk("\033[32m    Output: %s\033[0m\n", expected_output);
                sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN, SBI_SRST_RESET_REASON_NONE);
            }
            #endif
        }
    }
}

extern void __switch_to(struct task_struct *prev, struct task_struct *next);

void switch_to(struct task_struct *next) {
    // YOUR CODE HERE
    if(current->pid != next->pid){
        printk("\nswitch to [PID = %d PRIORITY = %d COUNTER = %d]\n", next->pid, next->priority, next->counter);
        struct task_struct* prev = current;
        current = next;
        __switch_to(prev, next);
    }
}

void do_timer() {
    // 1. 如果当前线程是 idle 线程或当前线程时间片耗尽则直接进行调度
    // 2. 否则对当前线程的运行剩余时间减 1，若剩余时间仍然大于 0 则直接返回，否则进行调度

    // YOUR CODE HERE
    if (current == idle || current->counter == 0) {
        schedule();
    }else{
        --(current->counter);
        if(current->counter > 0){
            return;
        }else{
            schedule();
        }
    }
}

void schedule() {
    uint64_t flag = 0;
    struct task_struct* next = idle;
    
	while (1) {
		for(int i=1; i < nr_tasks; i++){
            if(task[i]->state == TASK_RUNNING && task[i]->counter > flag){
                flag = task[i]->counter;
                next = task[i];
            }
        }
        if(flag != 0){
            break;
        }
        for(int i = 0;i < nr_tasks; i++){
                task[i]->counter = task[i]->priority;
                //printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", task[i]->pid, task[i]->priority, task[i]->counter);
        }
	}

    switch_to(next);
}




/*
* @mm       : current thread's mm_struct
* @addr     : the va to look up
*
* @return   : the VMA if found or NULL if not found
*/
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64_t addr)
{
    // 遍历VMA链表
    for (struct vm_area_struct *vma = mm->mmap; vma != NULL; vma = vma->vm_next)
    {
        // 检查addr是否在当前VMA的范围内
        if (addr >= vma->vm_start && addr < vma->vm_end) {
            return vma; // 找到目标VMA，返回指针
        }
    }

    // 没有找到匹配的VMA
    return NULL;
}


/*
* @mm       : current thread's mm_struct
* @addr     : the va to map
* @len      : memory size to map
* @vm_pgoff : phdr->p_offset
* @vm_filesz: phdr->p_filesz
* @flags    : flags for the new VMA
*
* @return   : start va
*/
void init_vma(struct vm_area_struct *vma, struct mm_struct *mm, uint64_t addr, uint64_t len, uint64_t vm_pgoff, uint64_t vm_filesz, uint64_t flags)
{
    vma->vm_pgoff = vm_pgoff;
    vma->vm_filesz = vm_filesz;
    vma->vm_mm = mm;
    vma->vm_start = addr;
    vma->vm_end = addr + len;
    vma->vm_flags = flags;
    vma->vm_next = NULL;
    vma->vm_prev = NULL;
}
void insert_vma_head(struct mm_struct *mm, struct vm_area_struct *tmp)
{
    if (mm->mmap != NULL) {
        struct vm_area_struct *ptr = mm->mmap;
        tmp->vm_next = ptr;
        ptr->vm_prev = tmp;
        mm->mmap = tmp; // 链表非空
    } else {
        mm->mmap = tmp;  // 如果链表为空,设为头部
    }
}
uint64_t do_mmap(struct mm_struct *mm, uint64_t addr, uint64_t len, uint64_t vm_pgoff, uint64_t vm_filesz, uint64_t flags)
{
    struct vm_area_struct *tmp=(struct vm_area_struct *)kalloc(sizeof(struct vm_area_struct));

    init_vma(tmp, mm, addr, len, vm_pgoff, vm_filesz, flags);

    insert_vma_head(mm, tmp);

    return tmp->vm_start;
}

