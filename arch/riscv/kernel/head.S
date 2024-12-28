    .extern start_kernel
    .extern mm_init
    .extern task_init
    .extern setup_vm # in vm.c
    .extern early_pgtbl # in vm.c
    .extern setup_vm_final # in vm.c
    .extern clock_set_next_event # in clock.c
    .section .text.init
    .globl _start
_start:
    # initialize stack
    la sp, boot_stack_top
    # set up vm
    jal setup_vm
    jal relocate
    # set stvec = _traps
    la t0, _traps
    csrw stvec, t0
    # initialize memory & task
    jal mm_init
    jal setup_vm_final
    jal task_init
    # set sie[STIE] = 1
    csrr t0, sie
    li t1, 0x00000020
    or t0, t0, t1
    csrw sie, t0
    # set first time interrupt
    jal clock_set_next_event
    # start kernel
    jal start_kernel

relocate:
    # t2 is used as offset
    li t0, 0xFFFFFFDF80000000
    add ra, ra, t0
    add sp, sp, t0
    la t1, early_pgtbl
    li t2, 12
    srl t1, t1, t2 # calculate PPN
    li t0, 8 # mode: Sv39
    li t2, 60
    sll t0, t0, t2 # calculate MODE
    add t0, t0, t1
    sfence.vma
    csrw satp, t0
    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 * 8 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top: