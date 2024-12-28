#include "stdint.h"
#include "printk.h"
#include "sbi.h"
#include "proc.h"
#include "defs.h"
#include "syscall.h"

void trap_handler(uint64_t scause, uint64_t sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断 trap 类型
    if ((scause & ((uint64_t)1 << 63)) == (uint64_t)1 << 63) {
        // 如果是 interrupt 判断是否是 timer interrupt
        if ((scause & (((uint64_t)1 << 63) - 1)) == 5) {
            // 如果是 timer interrupt 则打印输出相关信息，并通过 `clock_set_next_event()` 设置下一次时钟中断
            clock_set_next_event();
            do_timer();
        } else {
            // 其他 interrupt / exception 可以直接忽略，推荐打印出来供以后调试
            printk(BLUE "TRAP_HANDLER_MESSAGE: " CLEAR "unknown interrupt code: 0x%llx\n", (scause & (((uint64_t)1 << 63) - 1)));
        }
    } else {
        // 其他 interrupt / exception 可以直接忽略，推荐打印出来供以后调试
        // regs: = ra
        if (scause == (uint64_t)0x8) {
            printk(BLUE "TRAP_HANDLER_MESSAGE: " CLEAR "Environment call from U-mode, SYSCALL_ID = %d\n", regs -> x17);
            regs -> sepc += 4;
            if (regs -> x17 == SYS_WRITE) {
                printk(GREEN "U_MODE_MESSAGE: " CLEAR);
                regs -> x10 = sys_write(regs -> x10, (char *)regs -> x11, regs -> x12);
            }
            if (regs -> x17 == SYS_GETPID)
                regs -> x10 = sys_getpid();
            if (regs -> x17 == SYS_CLONE)
                regs -> x10 = do_fork(regs);
        } else if (scause == 12 || scause == 13 || scause == 15) {
            if (scause == 12)
                printk(BLUE "TRAP_HANDLER_MESSAGE: " CLEAR "Instruction Page Fault\n");
            if (scause == 13)
                printk(BLUE "TRAP_HANDLER_MESSAGE: " CLEAR "Load Page Fault\n");
            if (scause == 15) 
                printk(BLUE "TRAP_HANDLER_MESSAGE: " CLEAR "Store/AMO Page Fault\n");
            do_page_fault(regs);
        } else {
            printk(BLUE "TRAP_HANDLER_MESSAGE: " CLEAR "exception code: 0x%llx, sepc: 0x%llx\n", scause, sepc);
        }
    }
}