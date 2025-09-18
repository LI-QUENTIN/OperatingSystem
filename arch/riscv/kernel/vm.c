#include "mm.h"
#include "string.h"
#include "printk.h"
#include "defs.h"
#include "virtio.h"

/* early_pgtbl: 用于 setup_vm 进行 1GiB 的映射 */
uint64_t early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, int perm);

void setup_vm() {
    /* 
     * 1. 由于是进行 1GiB 的映射，这里不需要使用多级页表 
     * 2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
     *     high bit 可以忽略
     *     中间 9 bit 作为 early_pgtbl 的 index
     *     低 30 bit 作为页内偏移，这里注意到 30 = 9 + 9 + 12，即我们只使用根页表，根页表的每个 entry 都对应 1GiB 的区域
     * 3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    **/
    memset(early_pgtbl, 0x0, PGSIZE);
    uint64_t PA = PHY_START;
    uint64_t VA = VM_START;
    //early_pgtbl[VPN2(PA)] = ((PA >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X;
    early_pgtbl[VPN2(VA)] = ((PA >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X;
    printk("...setup_vm done!\n");
}

/* swapper_pg_dir: kernel pagetable 根目录，在 setup_vm_final 进行映射 */
uint64_t swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

extern char _stext[];
extern char _srodata[];
extern char _sdata[];


void setup_vm_final() {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, (uint64_t)_stext, (uint64_t)_stext - PA2VA_OFFSET, _srodata - _stext, PTE_X | PTE_R | PTE_V);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, (uint64_t)_srodata, (uint64_t)_srodata - PA2VA_OFFSET, _sdata - _srodata, PTE_R | PTE_V);

    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, (uint64_t)_sdata, (uint64_t)_sdata - PA2VA_OFFSET, PHY_SIZE - (_sdata - _stext), PTE_W | PTE_R | PTE_V);

    create_mapping(swapper_pg_dir, io_to_virt(VIRTIO_START), VIRTIO_START, VIRTIO_SIZE * VIRTIO_COUNT, PTE_W | PTE_R | PTE_V);

    // set satp with swapper_pg_dir
    uint64_t _satp = (((uint64_t)(swapper_pg_dir) - PA2VA_OFFSET) >> 12) | ((uint64_t)8 << 60);
    csr_write(satp, _satp);

    // flush TLB
    asm volatile("sfence.vma zero, zero");
    printk("...setup_vm_final done!\n");
    return;

}

uint64_t *get_or_create_pgtbl_entry(uint64_t *current_pgtbl, uint64_t vpn, uint64_t *new_pgtbl) {
    uint64_t *pgtbl;
    if (!(current_pgtbl[vpn] & PTE_V)) {
        new_pgtbl = (uint64_t *)kalloc();
        memset(new_pgtbl, 0, PGSIZE);
        current_pgtbl[vpn] = ((((uint64_t)new_pgtbl - PA2VA_OFFSET) >> 12) << 10) | PTE_V;
        pgtbl=new_pgtbl;
    }else{
        pgtbl = (uint64_t *)((((uint64_t)current_pgtbl[vpn] >> 10) << 12) + PA2VA_OFFSET);
    }
    return pgtbl;
}

void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, int perm) {
    /*
     * pgtbl 为根页表的基地址
     * va, pa 为需要映射的虚拟地址、物理地址
     * sz 为映射的大小，单位为字节
     * perm 为映射的权限（即页表项的低 8 位）
     * 
     * 创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
     * 可以使用 V bit 来判断页表项是否存在
    **/
    uint64_t *current_pgtbl, *new_pgtbl;
    uint64_t va_end = va + sz;

    while (va < va_end) {
        uint64_t vpn2 = VPN2(va), vpn1 = VPN1(va), vpn0 = VPN0(va);
        current_pgtbl = pgtbl;

        // 第一层页表 (VPN2)
        current_pgtbl = get_or_create_pgtbl_entry(current_pgtbl, vpn2, new_pgtbl);

        // 第二层页表 (VPN1)
        //current_pgtbl += (uint64_t)PA2VA_OFFSET;
        current_pgtbl = get_or_create_pgtbl_entry(current_pgtbl, vpn1, new_pgtbl);

        // 第三层页表 (VPN0) 和映射到物理地址
        //current_pgtbl += (uint64_t)PA2VA_OFFSET;
        current_pgtbl[vpn0] = ((pa >> 12) << 10) | perm | PTE_V;

        va += PGSIZE;
        pa += PGSIZE;
    }
}

