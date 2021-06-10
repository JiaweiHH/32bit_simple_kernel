#include "debug.h"

static void print_stack_trace();
static elf_t kernel_elf;

void init_debug() {
  kernel_elf = elf_from_mutilboot(glb_mboot_ptr);
}

void print_cur_status() {
  static int round = 0;
  uint16_t reg1, reg2, reg3, reg4;
  asm volatile("mov %%cs, %0;"
               "mov %%ds, %1;"
               "mov %%es, %2;"
               "mov %%ss, %3;"
               : "=m"(reg1), "=m"(reg2), "=m"(reg3), "=m"(reg4));
  // 打印当前的运行级别
  printk("%d: @ring %d\n", round, reg1 & 0x3);
  printk("%d:  cs = %x\n", round, reg1);
  printk("%d:  ds = %x\n", round, reg2);
  printk("%d:  es = %x\n", round, reg3);
  printk("%d:  ss = %x\n", round, reg4);
  ++round;
}

void panic(const char *msg) {
  printk("--------- System panic: %s ---------\n", msg);
  print_stack_trace();  // 追溯函数调用栈
  printk("---------\n");
  while(1); // 打印栈信息后停在这里
}

/**
 * 例如 start -> kernel_entry -> console_clear()
 * ebp[console_clear] ---> ebp[kernel_entry] ---> ebp[start]
 * 
 * ebp 保存的是上一个函数的 ebp 的值，ebp + 1 也就是上一个函数在调用下一个函数之前压入的下一条指令地址
 * 所以我们可以通过 ebp + 1 指针指向的内容找到上一个函数里面某一条指令的地址，再根据这个地址确定函数名字
 */
void print_stack_trace() {
  uint32_t *ebp, *eip;
  asm volatile ("mov %%ebp, %0" : "=r"(ebp));
  while(ebp) {
    eip = ebp + 1;
    printk("  [0x%x] %s\n", *eip, elf_lookup_symbol(*eip, &kernel_elf));
    ebp = (uint32_t *)*ebp;
  }
}