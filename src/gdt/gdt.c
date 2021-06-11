#include "gdt.h"
#include "mstring.h"

#define GDT_LENGTH 5

gdt_entry_t gdt_entries[GDT_LENGTH];
gdt_ptr_t gdt_ptr;

/**
 * @num: 段描述符索引
 * @base: 段描述符对应的基址
 * @limit: 段描述符表示的段的长度
 * @access: 访问标志，对应段描述符的第 6 个 byte
 * @gran: 其他访问标志
 */
static void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran);
/* 声明内核栈地址 */
extern uint32_t stack;

void init_gdt() {
  gdt_ptr.limit = sizeof(gdt_entry_t) * GDT_LENGTH - 1;
  gdt_ptr.base = (uint32_t)&gdt_entries;

  /* 采用平坦内存模型，设置段描述符 */
  gdt_set_gate(0, 0, 0, 0, 0);                  /* intel 文档规定第一个必须是 0 */
  gdt_set_gate(1, 0, 0xffffffff, 0x9A, 0xCF);   /* 指令段 */
  gdt_set_gate(2, 0, 0xffffffff, 0x92, 0xCF);   /* 数据段 */
  gdt_set_gate(3, 0, 0xffffffff, 0xFA, 0xCF);   /* 用户模式代码段 */
  gdt_set_gate(4, 0, 0xffffffff, 0xF2, 0xCF);   /* 用户模式数据段 */
  
  /* 加载全局描述符表到 GDTR 寄存器 */
  gdt_flush((uint32_t)&gdt_ptr);
}

static void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran) {
  /* 设置基址 */
  gdt_entries[num].base_low = base & 0xffff;
  gdt_entries[num].base_middle = (base >> 16) & 0xff;
  gdt_entries[num].base_high = (base >> 24) & 0xff;

  /* 设置限长 */
  gdt_entries[num].limit_low = limit & 0xffff;
  gdt_entries[num].granularty = (limit >> 16) & 0x0f;

  /* 设置标志位 */
  gdt_entries[num].granularty |= gran & 0xf0;
  gdt_entries[num].access = access;
}