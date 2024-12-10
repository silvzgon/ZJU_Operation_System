#include "sbi.h"
#include "defs.h"
#include "printk.h"

void test() {
    int i = 0;
    while (1) {
        if ((++i) % 100000000 == 0) {
            i = 0;
        }
    }
}
