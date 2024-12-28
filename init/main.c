#include "printk.h"
#include "proc.h"

extern void test();

int start_kernel() {
    printk("2024");
    printk(" ZJU Operating System\n");
    schedule();
    test();
    return 0;
}
