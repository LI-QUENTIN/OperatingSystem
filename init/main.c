#include "printk.h"
#include "defs.h"

extern void test();
extern void schedule();
extern char _stext[];
extern char _srodata[];


int start_kernel() {
    printk("2024");
    printk(" ZJU Operating System\n");
    // printk("sstatus: ");
    // printk("0x%lx\n", csr_read(sstatus));
    // csr_write(sscratch,5488);
    // printk("sscratch: ");
    // printk("%d\n", csr_read(sscratch));
    // printk("_stext = %ld\n", *_stext);
    // printk("_srodata = %ld\n", *_srodata);
    // *_stext = 0;
    // *_srodata = 0;
    // printk("write: _stext = %ld\n", *_stext);
    // printk("write: _srodata = %ld\n", *_srodata);
    schedule();
    test();
    return 0;
}
