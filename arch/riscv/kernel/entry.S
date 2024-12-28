    .extern trap_handler
    .extern dummy
    .globl __dummy
    .globl __switch_to
    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    # 1. save 32 registers and sepc to stack
    # 进入 trap 的时候需要切换到内核栈
    csrrw sp, sscratch, sp
    # 如果是内核线程（没有用户栈）触发了异常，则不需要进行切换
    bne sp, x0, traps_in_user_mode # caused by user mode
    csrrw sp, sscratch, sp
traps_in_user_mode:

    addi sp, sp, -8
    sd x1, 0(sp)
    addi sp, sp, -8
    sd x3, 0(sp)
    addi sp, sp, -8
    sd x4, 0(sp)
    addi sp, sp, -8
    sd x5, 0(sp)
    addi sp, sp, -8
    sd x6, 0(sp)
    addi sp, sp, -8
    sd x7, 0(sp)
    addi sp, sp, -8
    sd x8, 0(sp)
    addi sp, sp, -8
    sd x9, 0(sp)
    addi sp, sp, -8
    sd x10, 0(sp)
    addi sp, sp, -8
    sd x11, 0(sp)
    addi sp, sp, -8
    sd x12, 0(sp)
    addi sp, sp, -8
    sd x13, 0(sp)
    addi sp, sp, -8
    sd x14, 0(sp)
    addi sp, sp, -8
    sd x15, 0(sp)
    addi sp, sp, -8
    sd x16, 0(sp)
    addi sp, sp, -8
    sd x17, 0(sp)
    addi sp, sp, -8
    sd x18, 0(sp)
    addi sp, sp, -8
    sd x19, 0(sp)
    addi sp, sp, -8
    sd x20, 0(sp)
    addi sp, sp, -8
    sd x21, 0(sp)
    addi sp, sp, -8
    sd x22, 0(sp)
    addi sp, sp, -8
    sd x23, 0(sp)
    addi sp, sp, -8
    sd x24, 0(sp)
    addi sp, sp, -8
    sd x25, 0(sp)
    addi sp, sp, -8
    sd x26, 0(sp)
    addi sp, sp, -8
    sd x27, 0(sp)
    addi sp, sp, -8
    sd x28, 0(sp)
    addi sp, sp, -8
    sd x29, 0(sp)
    addi sp, sp, -8
    sd x30, 0(sp)
    addi sp, sp, -8
    sd x31, 0(sp)
    addi sp, sp, -8
    csrr t0, sepc
    sd t0, 0(sp)

    # 2. call trap_handler
    csrr a0, scause
    csrr a1, sepc
    addi a2, sp, 0
    jal trap_handler

    .globl __ret_from_fork
__ret_from_fork:
    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    ld t0, 0(sp)
    csrw sepc, t0
    addi sp, sp, 8
    ld x31, 0(sp)
    addi sp, sp, 8
    ld x30, 0(sp)
    addi sp, sp, 8
    ld x29, 0(sp)
    addi sp, sp, 8
    ld x28, 0(sp)
    addi sp, sp, 8
    ld x27, 0(sp)
    addi sp, sp, 8
    ld x26, 0(sp)
    addi sp, sp, 8
    ld x25, 0(sp)
    addi sp, sp, 8
    ld x24, 0(sp)
    addi sp, sp, 8
    ld x23, 0(sp)
    addi sp, sp, 8
    ld x22, 0(sp)
    addi sp, sp, 8
    ld x21, 0(sp)
    addi sp, sp, 8
    ld x20, 0(sp)
    addi sp, sp, 8
    ld x19, 0(sp)
    addi sp, sp, 8
    ld x18, 0(sp)
    addi sp, sp, 8
    ld x17, 0(sp)
    addi sp, sp, 8
    ld x16, 0(sp)
    addi sp, sp, 8
    ld x15, 0(sp)
    addi sp, sp, 8
    ld x14, 0(sp)
    addi sp, sp, 8
    ld x13, 0(sp)
    addi sp, sp, 8
    ld x12, 0(sp)
    addi sp, sp, 8
    ld x11, 0(sp)
    addi sp, sp, 8
    ld x10, 0(sp)
    addi sp, sp, 8
    ld x9, 0(sp)
    addi sp, sp, 8
    ld x8, 0(sp)
    addi sp, sp, 8
    ld x7, 0(sp)
    addi sp, sp, 8
    ld x6, 0(sp)
    addi sp, sp, 8
    ld x5, 0(sp)
    addi sp, sp, 8
    ld x4, 0(sp)
    addi sp, sp, 8
    ld x3, 0(sp)
    addi sp, sp, 8
    # ld x2, 0(sp)
    # addi sp, sp, 8
    ld x1, 0(sp)
    addi sp, sp, 8

    csrrw sp, sscratch, sp
    bne sp, x0, return_from_traps
    csrrw sp, sscratch, sp
return_from_traps:

    # 4. return from trap
    sret

__dummy:
    # swap sp and sscratch
    # 在 __dummy 进入用户态模式的时候，我们需要切换这两个栈
    csrrw sp, sscratch, sp
    sret

__switch_to:
    # save state to prev process
    add t0, a0, x0
    addi t0, t0, 32
    sd ra, 0(t0)
    sd sp, 8(t0)
    sd s0, 16(t0)
    sd s1, 24(t0)
    sd s2, 32(t0)
    sd s3, 40(t0)
    sd s4, 48(t0)
    sd s5, 56(t0)
    sd s6, 64(t0)
    sd s7, 72(t0)
    sd s8, 80(t0)
    sd s9, 88(t0)
    sd s10, 96(t0)
    sd s11, 104(t0)
    csrr t1, sepc
    sd t1, 112(t0)
    csrr t1, sstatus
    sd t1, 120(t0)
    csrr t1, sscratch
    sd t1, 128(t0)

    # restore state from next process
    add t0, a1, x0
    addi t0, t0, 32 # state, ..., pid
    ld ra, 0(t0)
    ld sp, 8(t0)
    ld s0, 16(t0)
    ld s1, 24(t0)
    ld s2, 32(t0)
    ld s3, 40(t0)
    ld s4, 48(t0)
    ld s5, 56(t0)
    ld s6, 64(t0)
    ld s7, 72(t0)
    ld s8, 80(t0)
    ld s9, 88(t0)
    ld s10, 96(t0)
    ld s11, 104(t0)
    ld t1, 112(t0)
    csrw sepc, t1
    ld t1, 120(t0)
    csrw sstatus, t1
    ld t1, 128(t0)
    csrw sscratch, t1

    ld t1, 136(t0) # pgd
    li t2, 12
    srl t1, t1, t2 # calculate PPN
    li t0, 8 # mode: Sv39
    li t2, 60
    sll t0, t0, t2 # calculate MODE
    add t0, t0, t1
    csrw satp, t0
    sfence.vma

    ret