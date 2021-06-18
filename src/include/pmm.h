#ifndef INCLUDE_PMM_H_
#define INCLUDE_PMM_H_

#include "mutilboot.h"

#define STACK_SIZE 8192           /* 线程栈的大小 */
#define PMM_MAX_SIZE 0x20000000   /* 支持的最大物理内存 512MB */
#define PMM_PAGE_SIZE 0x1000      /* 页框大小 4KB */
#define PAGE_MAX_SIZE (PMM_MAX_SIZE / PMM_PAGE_SIZE)  /* 最多支持的物理页面数量 */
#define PHY_PAGE_MASK 0xfffff000  /* 掩码，按照 4KB 对齐 */

/* 内核文件在内存中的起始地址和结束地址，在链接脚本中由链接器定义 */
extern uint8_t kern_start[];
extern uint8_t kern_end[];

/* 内核栈的栈顶 */
extern uint32_t kern_stack_top;

/* 动态分配物理内存页的总数 */
extern uint32_t phy_page_count;

/* 输出 BIOS 提供的物理内存布局 */
void show_memory_map();

/* 初始化物理内存管理 */
void init_pmm();
/* 返回一个内存页的物理地址 */
uint32_t pmm_alloc_page();
/* 释放申请的内存 */
void pmm_free_page(uint32_t);

#endif