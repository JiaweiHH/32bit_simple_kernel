#include "console.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"

/* 内核初始化函数 */
void kern_init();

/* 开启分页之后的 mutilboot 数据指针 */
mutilboot_t *glb_mboot_ptr;

/* 开启分页之后的内核栈 */
char kern_stack[STACK_SIZE];

/* 内核使用的临时页表和页目录，保存在 1MB 以下的内存地址 */
__attribute__((section(".init.data"))) pgd_t *pgd_tmp   = (pgd_t *)0x1000;
__attribute__((section(".init.data"))) pgd_t *pte_low   = (pgd_t *)0x2000;
__attribute__((section(".init.data"))) pgd_t *pte_high  = (pgd_t *)0x3000;

/* 内核入口函数 */
__attribute__((section(".init.text"))) void kern_entry() {
  /**
   * 设置 pgd 的两个表项，最低的和 3GB 位置的表项
   * pgd 的第一个表项保存的是第一个 pte 的地址也就是 pte_low，pte_low 的 1024 个表项保存了 4MB 的物理地址映射
   * pgd 的 PGD_INDEX(PAGE_OFFSET) 对应的表项保存的是 0xC0000000 的 pte 地址，也就是 pte_high
   */
  pgd_tmp[0] = (uint32_t)pte_low | PAGE_PRESENT | PAGE_WRITE;
  pgd_tmp[PGD_INDEX(PAGE_OFFSET)] = (uint32_t)pte_high | PAGE_PRESENT | PAGE_WRITE;

  /**
   * 映射 0x00000000-0x00400000 的物理地址到虚拟地址 0x00000000-0x00400000；
   * 映射 0x00000000-0x00400000 的物理地址到虚拟地址 0xC0000000-0xC0400000；
   * 设置它们的页表项，页表项 = 物理页号 20bit + 标志位 12bit
   * 这里总共映射了 1024 个物理页，也就是 4MB
   * 
   * 这里虚拟地址的低 4MB 和 3GB + 4MB 都映射到物理地址的 4MB 处，
   * 这是因为在 kern_entry 函数中会开启分页，
   * 如果没有在低 4MB 映射的话在开启分页之后 kern_entry 函数的代码就会出错，因为地址没有映射了
   */
  int i;
  for(i = 0; i < PTE_SIZE; ++i) {
    pte_low[i] = (i << 12) | PAGE_PRESENT | PAGE_WRITE;
    pte_high[i] = (i << 12) | PAGE_PRESENT | PAGE_WRITE;
  }

  /* 设置临时页表，更改 cr3 寄存器 */
  asm volatile ("mov %0, %%cr3" : : "r"(pgd_tmp));

  /* 启用分页，将 cr0 寄存器的分页 bit 置为 1 */
  uint32_t cr0;
  asm volatile ("mov %%cr0, %0" : "=r"(cr0));   // 读取 cr0 寄存器的值存储在 cr0 变量中
  cr0 |= 0x80000000;                            // 设置 bit
  asm volatile ("mov %0, %%cr0" : : "r"(cr0));  // 将 cr0 变量的值设置在 cr0 寄存器中

  /* 切换内核栈 */
  uint32_t kern_stack_top = ((uint32_t)kern_stack + STACK_SIZE) & 0xfffffff0;
  asm volatile (
    "mov %0, %%esp\n\t"
    "xor %%ebp, %%esp" :: "r"(kern_stack_top)
  );  // 设置 kern_stack_top 的值到 esp 寄存器，esp = esp ^ ebp
  
  /** 
   * 更新全局 mutilboot_t 指针，
   * mboot_ptr_tmp 指针存储的地址是物理地址，
   * 加上 PAGE_OFFSET 变成现在的内核地址用来做访问
   */
  glb_mboot_ptr = mboot_ptr_tmp + PAGE_OFFSET;

  kern_init();
}

void kern_init() {
  init_debug();
  init_gdt();
  init_idt();

  console_clear();
  printk_color(rc_black, rc_green, "Hello, OS kernel~~~\n");

  init_timer(200);
  // asm volatile("sti");

  printk("kernel in memory start: 0x%08X\n", kern_start);
  printk("kernel in memory end:   0x%08X\n", kern_end);
  printk("kernel in memory used:   %d KB\n\n", (kern_end - kern_start + 1023) / 1024);
  show_memory_map();

  init_pmm();
  init_vmm();
  init_heap();

  printk_color(rc_black, rc_red, "\nThe Count of Physical Memory Page is: %u\n\n", phy_page_count);
  test_heap();

  while(1) {
    asm volatile ("hlt");
  }
}