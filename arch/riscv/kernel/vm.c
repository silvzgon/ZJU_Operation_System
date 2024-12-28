#include "stdint.h"
#include "printk.h"
#include "defs.h"
#include "string.h"
#include "mm.h"
#include "vm.h"
#include "proc.h"

extern uint64_t _stext;
extern uint64_t _etext;
extern uint64_t _srodata;
extern uint64_t _erodata;
extern uint64_t _sdata;
extern uint64_t _edata;

extern char _sramdisk[];
extern char _eramdisk[];

extern struct task_struct *current;

/* early_pgtbl: map memory from 0x80000000 to 0xffffffe000000000 */
uint64_t early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm() {
    /* 
     * 1. 由于是进行 1GiB 的映射，这里不需要使用多级页表 
     * 2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
     *     high bit 可以忽略
     *     中间 9 bit 作为 early_pgtbl 的 index
     *     低 30 bit 作为页内偏移，这里注意到 30 = 9 + 9 + 12，即我们只使用根页表，根页表的每个 entry 都对应 1GiB 的区域
     * 3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    early_pgtbl[(PHY_START >> 30) % 512] = ((PHY_START >> 12 << 10) & (((uint64_t)1 << 54) - 1)) | 0xf;
    early_pgtbl[(VM_START  >> 30) % 512] = ((PHY_START >> 12 << 10) & (((uint64_t)1 << 54) - 1)) | 0xf;
    printk("...setup_vm done!\n");
}

/* swapper_pg_dir: kernel pagetable 根目录，在 setup_vm_final 进行映射 */
uint64_t swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final() {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required
    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, (uint64_t)&_stext, (uint64_t)&_stext - PA2VA_OFFSET, (uint64_t)&_etext - (uint64_t)&_stext, PGTBL_X | PGTBL_R | PGTBL_VALID);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, (uint64_t)&_srodata, (uint64_t)&_srodata - PA2VA_OFFSET, (uint64_t)&_erodata - (uint64_t)&_srodata, PGTBL_R | PGTBL_VALID);

    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, (uint64_t)&_sdata, (uint64_t)&_sdata - PA2VA_OFFSET, (uint64_t)PHY_END + (uint64_t)PA2VA_OFFSET - (uint64_t)&_sdata, PGTBL_W | PGTBL_R | PGTBL_VALID);

    // set satp with swapper_pg_dir
    csr_write(satp, (((uint64_t)swapper_pg_dir - PA2VA_OFFSET)>> 12) & (((uint64_t)1 << 44) - 1) | ((uint64_t)8 << 60));

    // flush TLB
    asm volatile("sfence.vma zero, zero");
    printk("...setup_vm_final done!\n");
    return;
}


/* 创建多级页表映射关系 */
/* 不要修改该接口的参数和返回值 */
void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, uint64_t perm) {
    /*
     * pgtbl 为根页表的基地址
     * va, pa 为需要映射的虚拟地址、物理地址
     * sz 为映射的大小，单位为字节
     * perm 为映射的权限（即页表项的低 8 位）_stext
     * 
     * 创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
     * 可以使用 V bit 来判断页表项是否存在
    */
    pgtbl = ((uint64_t)pgtbl & 0x8000000000000000U) ? pgtbl : (uint64_t *)((uint64_t)pgtbl + PA2VA_OFFSET);
    uint64_t size = PGROUNDUP(sz) >> 12;
    uint64_t offset, *base_address, *temp;
    for (uint64_t idx = 0; idx < size; idx++, va += 4096, pa += 4096) {
        base_address = pgtbl;
        offset = (va >> 30) & (((uint64_t)1 << 9) - 1);
        if ((base_address[offset] & PGTBL_VALID) == 0) {
            temp = (uint64_t *)kalloc();
            base_address[offset] = PGTBL_VALID | (((uint64_t)temp - PA2VA_OFFSET) >> 12 << 10);
        }
        base_address = (uint64_t *)((base_address[offset] >> 10 << 12) + PA2VA_OFFSET);
        offset = (va >> 21) & (((uint64_t)1 << 9) - 1);
        if ((base_address[offset] & PGTBL_VALID) == 0) {
            temp = (uint64_t *)kalloc();
            base_address[offset] = PGTBL_VALID | (((uint64_t)temp - PA2VA_OFFSET) >> 12 << 10);
        }
        base_address = (uint64_t *)((base_address[offset] >> 10 << 12) + PA2VA_OFFSET);
        offset = (va >> 12) & (((uint64_t)1 << 9) - 1);
        base_address[offset] = perm | ((uint64_t)pa >> 12 << 10);
    }
}

// 实现对 vm_area_struct 的查找
struct vm_area_struct *find_vma(struct mm_struct *mm, uint64_t addr) {
    struct vm_area_struct *vma = mm -> mmap;
    // 根据传入的地址 addr，遍历链表 mm 包含的 VMA 链表，找到该地址所在的 vm_area_struct
    while (vma) {
        if (addr >= vma -> vm_start && addr < vma -> vm_end)
            return vma;
        else
            vma = vma -> vm_next;
    }
    // 如果链表中所有的 vm_area_struct 都不包含该地址，则返回 NULL
    return NULL;
}

// 实现 vm_area_struct 的添加
uint64_t do_mmap(struct mm_struct *vm_mm, uint64_t addr, uint64_t len, uint64_t vm_pgoff, uint64_t vm_filesz, uint64_t vm_flags) {
    // 新建 vm_area_struct 结构体
    struct vm_area_struct* vma = (struct vm_area_struct *)kalloc();
    // 根据传入的参数对结构体赋值，并添加到 mm 指向的 VMA 链表中
    vma -> vm_next = vm_mm -> mmap;
    vma -> vm_prev = NULL;
    if (vm_mm -> mmap)
        vm_mm -> mmap -> vm_prev = vma;
    vma -> vm_mm = vm_mm;
    vma -> vm_start = addr;
    vma -> vm_end = (uint64_t)addr + (uint64_t)len;
    vma -> vm_flags = vm_flags;
    vma -> vm_pgoff = vm_pgoff;
    vma -> vm_filesz = vm_filesz;
    vm_mm -> mmap = vma;
    return addr;
}

void do_page_fault(struct pt_regs *regs) {
    // 通过 stval 获得访问出错的虚拟内存地址（Bad Address）
    uint64_t stval  = csr_read(stval);
    uint64_t scause = csr_read(scause);
    // 通过 find_vma() 查找 bad address 是否在某个 vma 中
    struct vm_area_struct *vma = find_vma(&(current -> mm), stval);
    if (vma == (struct vm_area_struct*)NULL) {
        // 如果不在，则出现非预期错误，可以通过 Err 宏输出错误信息
        Err("Unexpected error: bad address not in vma.");
    }
    // 如果在，则根据 vma 的 flags 权限判断当前 page fault 是否合法
    if (
        (scause == 12) && (!((vma -> vm_flags) & VM_EXEC)) ||
        (scause == 13) && (!((vma -> vm_flags) & VM_READ)) ||
        (scause == 15) && (!((vma -> vm_flags) & VM_WRITE))
    ) {
        // 如果非法（比如触发的是 instruction page fault 但 vma 权限不允许执行），则 Err 输出错误信息
        Err("Unexpected error: operation illegal");
    }
    // 其他情况合法，需要我们按接下来的流程创建映射
    // 分配一个页，接下来要将这个页映射到对应的用户地址空间
    uint64_t perm = PGTBL_VALID | PGTBL_U |
        ((vma -> vm_flags & VM_EXEC)  ? PGTBL_X : 0) |
        ((vma -> vm_flags & VM_WRITE) ? PGTBL_W : 0) |
        ((vma -> vm_flags & VM_READ)  ? PGTBL_R : 0);
    // 通过 (vma->vm_flags & VM_ANON) 获得当前的 VMA 是否是匿名空间
    if (vma -> vm_flags & VM_ANON) {
        // 如果是匿名空间，则直接映射即可
        create_mapping(current->pgd, PGROUNDDOWN(stval), (uint64_t)kalloc() - PA2VA_OFFSET, PGSIZE, perm);
    } else {
        // 如果不是，则需要根据 vma->vm_pgoff 等信息从 ELF 中读取数据，填充后映射到用户空间
        char*    target_page  = kalloc();
        uint64_t round_down   = PGROUNDDOWN(stval);
        char*    f_file_begin = (char*)((uint64_t)_sramdisk + vma->vm_pgoff);
        char*    f_file_end   = f_file_begin + vma->vm_filesz;
        char*    f_mem_end    = f_file_begin + (vma->vm_end - vma->vm_start);
        for (uint64_t i = 0; i < PGSIZE; i++) {
            char *target_position = f_file_begin + (round_down + i - vma->vm_start);
            if (target_position >= f_file_begin && target_position < f_file_end)
                target_page[i] = *target_position;
            else
                target_page[i] = 0;
        }
        create_mapping(current->pgd, PGROUNDDOWN(stval), (uint64_t)target_page - PA2VA_OFFSET, PGSIZE, perm);
    }
}