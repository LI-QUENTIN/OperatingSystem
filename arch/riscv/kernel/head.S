    .extern start_kernel
    # .extern virtio_dev_init
    # .extern mbr_init
    .section .text.init
    .globl _start
_start:
    # ------------------
    # - your code here -
    # ------------------

    la sp, boot_stack_top

    call setup_vm
    call relocate
    # memory init
    call mm_init

    call setup_vm_final
    # task init
    call task_init
    # call virtio_dev_init
    # call mbr_init
    # set stvec = _traps 
    la t0, _traps
    csrw stvec, t0
    # set sie[STIE] = 1
    csrr t0, sie
	ori t0, t0, 1<<5  
	csrw sie, t0
    # set first time interrupt 
    rdtime a0
    li t0, 10000000
    add a0, a0, t0
    call sbi_set_timer
    # set sstatus[SIE] = 1
    #csrr t0, sstatus
    #ori t0, t0, 1<<1
    #csrw sstatus, t0
    # jump to start_kernel
    jal ra,start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    li t0, 0xffffffdf80000000
    add ra, ra, t0 
    add sp, sp, t0 

    # flush tlb
    sfence.vma zero, zero

    la t1, position
    add t1, t1, t0
    csrw stvec, t1
    
    # set satp with early_pgtbl
    la t0, early_pgtbl
    srli t0, t0, 12
    li t1, 0x8000000000000000
    or t0, t0, t1
    csrw satp, t0

position: 
    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top: