    .extern start_kernel
    .section .text.init
    .globl _start
_start:
    # (previous) initialize stack
	la sp,boot_stack_top # initial the sp

    call setup_vm
    call relocate

    call mm_init
    call task_init

    call setup_vm_final

    # set stvec = _traps
	la t0, _traps
	csrw stvec, t0 # csrrw is reg2reg, while _traps is a label containing a mem_addr

	li t0, 1<<5 # STIE is the 5_bit of sie
	csrs sie, t0 # csrs = csrs or t0
    
    rdtime a0
	li a7, 0x54494d45 # eid
    li a6, 0 # fid
    li t0, 10000000 # 1s
    add a0, a0, t0
    ecall # call sbi_set_timer
	
    # set sstatus[SIE] = 1
    # li t0, 1<<1  # the 1_bit
	csrsi sstatus, 1<<1

    # (previous) jump to start_kernel
	call start_kernel # goto start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)

    addi t0, x0, 1
    slli t0, t0, 31 # 0x80000000

    lui t1, 0xfffff
    li t2, 0xfdf
    add t1, t1, t2

    slli t1, t1, 32 # 0xffffffdf00000000
    add t0, t0, t1  # PA2VA_OFFSET

    add ra, ra, t0   # Add PA2VA_OFFSET to ra
    add sp, sp, t0   # Add PA2VA_OFFSET to sp

    sfence.vma zero, zero   
    
    la t1, early_pgtbl
    # sub t1, t1, t0  # t0 = PA2VA_OFFSET
    srli t1, t1, 12 # PA >> 12 == PPN

    li t2, 8    # set mode = Sv39
    slli t2, t2, 60
    
    or t1, t1, t2
    csrw satp, t1

    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # leave enough stack space for start_kernel

    .globl boot_stack_top
boot_stack_top: