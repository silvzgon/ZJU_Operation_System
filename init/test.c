#include "printk.h"
#include "sbi.h"

void test() {
    int i = 0;
    while (1) {
        if ((++i) % 100000000 == 0) {
            printk("kernel is running! time: %llu\n", get_cycles());
            i = 0;
        }
    }
}