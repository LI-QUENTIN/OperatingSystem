    .extern trap_handler
    .section .text.entry
    .align 2
    .globl _traps 

_traps:
    csrr t0, sscratch
    beq t0, x0, _traps_save
    csrw sscratch, sp
    mv sp, t0
_traps_save:
    # 1. save 32 registers and sepc to stack
    addi sp, sp, -35*8
    csrr t0, stval
    sd t0, 34*8(sp)
    csrr t0, sstatus
    sd t0, 33*8(sp)
    csrr t0, sepc
    sd t0, 32*8(sp)
    sd x31, 31*8(sp)
    sd x30, 30*8(sp)
    sd x29, 29*8(sp)
    sd x28, 28*8(sp)
    sd x27, 27*8(sp)
    sd x26, 26*8(sp)
    sd x25, 25*8(sp)
    sd x24, 24*8(sp)
    sd x23, 23*8(sp)
    sd x22, 22*8(sp)
    sd x21, 21*8(sp)
    sd x20, 20*8(sp)
    sd x19, 19*8(sp)
    sd x18, 18*8(sp)
    sd x17, 17*8(sp)
    sd x16, 16*8(sp)
    sd x15, 15*8(sp)
    sd x14, 14*8(sp)
    sd x13, 13*8(sp)
    sd x12, 12*8(sp)
    sd x11, 11*8(sp)
    sd x10, 10*8(sp)
    sd x9, 9*8(sp)
    sd x8, 8*8(sp)
    sd x7, 7*8(sp)
    sd x6, 6*8(sp)
    sd x5, 5*8(sp)
    sd x4, 4*8(sp)
    sd x3, 3*8(sp)
    sd x2, 2*8(sp)
    sd x1, 1*8(sp)
    sd x0, 0*8(sp)
    
    # 2. call trap_handler
    csrr a0, scause
    csrr a1, sepc
    mv a2, sp
    call trap_handler

    .globl __ret_from_fork
__ret_from_fork:
    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    ld x0, 0*8(sp)
    ld x1, 1*8(sp)
    ld x3, 3*8(sp)
    ld x4, 4*8(sp)
    ld x5, 5*8(sp)
    ld x6, 6*8(sp)
    ld x7, 7*8(sp)
    ld x8, 8*8(sp)
    ld x9, 9*8(sp)
    ld x10, 10*8(sp)
    ld x11, 11*8(sp)
    ld x12, 12*8(sp)
    ld x13, 13*8(sp)
    ld x14, 14*8(sp)
    ld x15, 15*8(sp)
    ld x16, 16*8(sp)
    ld x17, 17*8(sp)
    ld x18, 18*8(sp)
    ld x19, 19*8(sp)
    ld x20, 20*8(sp)
    ld x21, 21*8(sp)
    ld x22, 22*8(sp)
    ld x23, 23*8(sp)
    ld x24, 24*8(sp)
    ld x25, 25*8(sp)
    ld x26, 26*8(sp)
    ld x27, 27*8(sp)
    ld x28, 28*8(sp)
    ld x29, 29*8(sp)
    ld x30, 30*8(sp)
    ld x31, 31*8(sp)
    ld t0, 32*8(sp)
    csrw sepc, t0
    ld t0, 33*8(sp)
    csrw sstatus, t0
    ld t0, 34*8(sp)
    csrw stval, t0
    ld x2, 2*8(sp)
    addi sp, sp, 35*8
    # 4. return from trap
    csrr t0, sscratch
    beq t0, x0, _traps_sret
    csrw sscratch, sp
    mv sp, t0
_traps_sret:
    sret


#__dummy
    .extern dummy
    .globl __dummy
__dummy:
    #la t0, dummy
    #csrw sepc,t0
    csrr t0, sscratch
    csrw sscratch, sp
    mv sp, t0

    sret

    

#__switch_to
    .extern __switch_to
    .globl __switch_to
__switch_to:
    
    # YOUR CODE HERE
    # save state to prev process
	sd ra, 4*8(a0) # Offset of thread_struct = 32
    sd sp, 5*8(a0)
    sd s0, 6*8(a0)
    sd s1, 7*8(a0)
    sd s2, 8*8(a0)
    sd s3, 9*8(a0)
    sd s4, 10*8(a0)
    sd s5, 11*8(a0)
    sd s6, 12*8(a0)
    sd s7, 13*8(a0)
    sd s8, 14*8(a0)
    sd s9, 15*8(a0)
    sd s10, 16*8(a0)
    sd s11, 17*8(a0)
    #save sepc,sstatus,sscratch
    csrr a2,sepc
    sd a2,18*8(a0)
    csrr a2,sstatus
    sd a2,19*8(a0)
    csrr a2,sscratch
    sd a2,20*8(a0)


    # restore state from next process
	ld ra, 4*8(a1) # Offset of thread_struct = 32
    ld sp, 5*8(a1)
    ld s0, 6*8(a1)
    ld s1, 7*8(a1)
    ld s2, 8*8(a1)
    ld s3, 9*8(a1)
    ld s4, 10*8(a1)
    ld s5, 11*8(a1)
    ld s6, 12*8(a1)
    ld s7, 13*8(a1)
    ld s8, 14*8(a1)
    ld s9, 15*8(a1)
    ld s10, 16*8(a1)
    ld s11, 17*8(a1) 

    #restore sepc,sstatus,sscratch
    ld a2,18*8(a1)
    csrw sepc,a2
    ld a2,19*8(a1)
    csrw sstatus,a2
    ld a2,20*8(a1)
    csrw sscratch,a2

    #switch page table
    ld a2,21*8(a1)
    li a3,0xffffffdf80000000   
    sub a2,a2,a3                
    srli a2,a2,12               
    li a3, 0x8000000000000000
    or a2,a2,a3
    csrw satp,a2                  

    #flash TLB and ICache
    sfence.vma zero, zero

    ret
