#ifndef INCLUDE_IDT_H_
#define INCLUDE_IDT_H_

#include "types.h"

void init_idt();

/* 中断描述符 */
typedef struct idt_entry_t {
  uint16_t base_lo; /* 中断服务例程地址 15 ~ 0bit */
  uint16_t sel;     /* 目标代码 段描述符选择子，从 GDT 中取得保存中断服务例程的段描述符 */
  uint8_t always0;  /* 置 0 段 */
  uint8_t flags;    /* 标志位 */
  uint16_t base_hi; /* 中断服务例程地址 31 ~ 16bit */
} __attribute__((packed)) idt_entry_t;

/* IDTR 寄存器，结构和 GDTR 一样 */
typedef struct idt_ptr_t {
  uint16_t limit; /* 限长 */
  uint32_t base;  /* 基址 */
} __attribute__((packed)) idt_ptr_t;

typedef struct pt_regs_t {
  uint32_t ds;          /* 保存用户的数据段描述符 */
  uint32_t edi;         /* edi 到 eax 由 pusha 指令压入 */
  uint32_t esi;
  uint32_t ebp;
  uint32_t esp;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;
  uint32_t int_no;      /* 中断号 */
  uint32_t err_code;    /* 错误代码 */
  uint32_t eip;         /* 以下由 cpu 自动压入 */
  uint32_t cs;
  uint32_t eflags;
  uint32_t user_esp;
  uint32_t ss;
} __attribute__((packed)) pt_regs;

/* 中断处理函数 */
typedef void (*interrupt_handler_t)(pt_regs *);
/* 注册中断处理函数 */
void register_interrupt_handler(uint8_t, interrupt_handler_t);
/* 调用中断处理函数 (interrupt service routine) */
void isr_handler(pt_regs *);

/** 
 * 声明异常处理函数
 * 0 ~ 19 属于 cpu 异常中断，20 ~ 31 被 intel 保留，32 ~ 255 属于用户自定义中断包括硬件终端
 */
void isr0();        /* 0 #DE 除 0 异常 */
void isr1();        /* 1 #DB 调试异常 */
void isr2();        /* 2 NMI */
void isr3();        /* 3 BP 断点异常 */
void isr4();        /* 4 #OF 溢出 */
void isr5();        /* 5 #BR 对数组的引用超出边界 */
void isr6();        /* 6 #UD 无效或未定义的操作码 */
void isr7();        /* 7 #NM 设备不可用(无数学协处理器) */
void isr8();        /* 8 #DF 双重故障(有错误代码) */
void isr9();        /* 9 协处理器跨段操作 */
void isr10();       /* 10 #TS 无效TSS(有错误代码) */
void isr11();       /* 11 #NP 段不存在(有错误代码) */
void isr12();       /* 12 #SS 栈错误(有错误代码) */
void isr13();       /* 13 #GP 常规保护(有错误代码) */
void isr14();       /* 14 #PF 页故障(有错误代码) */
void isr15();       /* 15 CPU 保留 */
void isr16();       /* 16 #MF 浮点处理单元错误 */
void isr17();       /* 17 #AC 对齐检查 */
void isr18();       /* 18 #MC 机器检查 */
void isr19();       /* 19 #XM SIMD(单指令多数据)浮点异常 */

void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();

// 32 ~ 255 用户自定义
void isr255();

/* IRQ 处理函数 */
void irq_handler(pt_regs *regs);
/* 定义 IRQ */
#define IRQ0 32     /* 计时器 */
#define IRQ1 33     /* 键盘 */
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

/* irq 和 isr 一样，都是通过 macro 声明和定义 */
void irq0();
void irq1();
void irq2();
void irq3();
void irq4();
void irq5();
void irq6();
void irq7();
void irq8();
void irq9();
void irq10();
void irq11();
void irq12();
void irq13();
void irq14();
void irq15();

/**
 * IRQ 和 ISR 处理过程很相似
 * ISR 处理过程：isri -> isr_common_stub -> isr_handler -> 具体的处理函数
 * IRQ 处理过程：irqi -> irq_common_stub -> irq_handler -> 具体的处理函数
 */

#endif