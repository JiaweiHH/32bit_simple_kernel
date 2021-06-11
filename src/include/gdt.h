#ifndef INCLUDE_GDT_H_
#define INCLUDE_GDT_H_

#include "types.h"

/* 段描述符 */
typedef struct gdt_entry_t {
  uint16_t limit_low;   /* 段界限 15 ~ 0 */
  uint16_t base_low;    /* 段基址 15 ~ 0 */
  uint8_t base_middle;  /* 段基址 23 ～ 16 */
  uint8_t access;       /* 段存在位、描述符特权级、描述符类型、描述符子类别 */
  uint8_t granularty;   /* 段界限 19 ～ 16 和 其他标志位 */
  uint8_t base_high;    /* 段基址 31 ～ 24 */
} __attribute__((packed)) gdt_entry_t;

/* GDTR 寄存器，存放的是 GDT 的首地址，该寄存器是 48bit 的 */
typedef struct gdt_ptr_t {
  uint16_t limit;   /* 全局描述符表长度 */
  uint32_t base;    /* 全局描述符表基址 */
} __attribute__((packed)) gdt_ptr_t;

/* 初始化全局描述符表 */
void init_gdt();

/* GDT 加载到 GDTR 的函数 */
extern void gdt_flush(uint32_t);

#endif