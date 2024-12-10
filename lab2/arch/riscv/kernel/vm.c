#include "defs.h"
#include "printk.h"
#include "mm.h"

typedef unsigned long long uint64;
extern char _stext[];
extern char _srodata[];
extern char _sdata[];
extern char _ekernel[];

/* early_pgtbl: 用于 setup_vm 进行 1GiB 的映射 */
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /* 
     * 1. 由于是进行 1GiB 的映射，这里不需要使用多级页表 
     * 2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
     *     high bit 可以忽略
     *     中间 9 bit 作为 early_pgtbl 的 index
     *     低 30 bit 作为页内偏移，这里注意到 30 = 9 + 9 + 12，即我们只使用根页表，根页表的每个 entry 都对应 1GiB 的区域
     * 3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    **/
    printk("...running setup_vm\n");

    uint64 index = PHY_START >> 30;
    index = index & 0x1ff;
    early_pgtbl[index] = PHY_START >> 2 | 0xf;

    index = (PHY_START + PA2VA_OFFSET) >> 30;
    index = index & 0x1ff;
    early_pgtbl[index] = PHY_START >> 2 | 0xf;
}

/* swapper_pg_dir: kernel pagetable 根目录，在 setup_vm_final 进行映射 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final(void) {
    printk("...running setup_vm_final\n");
    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, (uint64)_stext, (uint64)_stext - PA2VA_OFFSET, (uint64)_srodata - (uint64)_stext, 0b1011);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, (uint64)_srodata, (uint64)_srodata - PA2VA_OFFSET, (uint64)_sdata - (uint64)_srodata, 0b0011);

    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, (uint64)_sdata, (uint64)_sdata - PA2VA_OFFSET, PHY_SIZE - OPENSBI_SIZE - ((uint64)_sdata - (uint64)_stext), 0b0111);

    // set satp with swapper_pg_dir
    __asm__ volatile(
        "csrw satp, %0" :: "r"((uint64_t)swapper_pg_dir)
    );

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    return;
}


/* 创建多级页表映射关系 */
/* 不要修改该接口的参数和返回值 */
create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm){
    /*
     * pgtbl 为根页表的基地址
     * va, pa 为需要映射的虚拟地址、物理地址
     * sz 为映射的大小，单位为字节
     * perm 为映射的权限（即页表项的低 8 位）
     * 
     * 创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
     * 可以使用 V bit 来判断页表项是否存在
    **/
    uint64 offset[3];
    uint64 page[3];
    unsigned long *pgt_next = NULL;
    unsigned long *pgt_nnext = NULL;
    for (uint64 i = 0; i < sz; i += PGSIZE){
        offset[2] = (va >> 30) & 0x1FF;
        offset[1] = (va >> 21) & 0x1FF;
        offset[0] = (va >> 12) & 0x1FF;

        if (!(pgtbl[offset[2]] & 0x1)){
            pgt_next = (unsigned long *)kalloc();
            pgt_nnext = (unsigned long *)kalloc();
            page[2] = (uint64)pgt_next - PA2VA_OFFSET;
            page[2] = page[2] >> 12;
            page[2] = page[2] << 10;
            pgtbl[offset[2]] = page[2] | 0x0000000000000001;
        }
        else if (pgt_next == 0){
            pgt_next = (uint64 *)(((pgtbl[offset[2]] >> 10) << 12));
        }

        if (!(pgt_next[offset[1]] & 0x1)){
            pgt_nnext = (unsigned long *)kalloc();
            page[1] = (uint64)pgt_nnext - PA2VA_OFFSET;
            page[1] = page[1] >> 12;
            page[1] = page[1] << 10;
            pgt_next[offset[1]] = (page[1]) | 0x0000000000000001;
        }
        else if (pgt_nnext == 0){
            pgt_nnext = (uint64 *)(((pgt_next[offset[1]] >> 10) << 12));
        }

        page[0] = pa >> 12;
        page[0] = page[0] <<10;
        pgt_nnext[offset[0]] = page[0] | perm;
        va += PGSIZE;
        pa += PGSIZE;
    }
}