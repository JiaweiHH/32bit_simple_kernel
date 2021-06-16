#ifndef INCLUDE_VMM_H_
#define INCLUDE_VMM_H_

#include "types.h"
#include "idt.h"

#define PAGE_OFFSET 0xC0000000  /* 内核偏移地址 */

/**
 * P 位是存在标志，用于指明表项对地址转换是否有效
 * P = 1 表示有效，P = 0 表示无效
 * 在页转换过程中如果页目录或页表的表项无效，会导致异常
 * P = 0，除了说明表项无效之外，其余位可以供程序自由使用，例如 OS 可以使用这些 bit 来保存已存储在磁盘上的页面的序号
 */
#define PAGE_PRESENT 0x1

/**
 * R/W 位是读写标志
 * =1 表示页面可以被读、写或执行；=0 表示页面只读或可执行
 * 当处理器运行在特权级时 R/W bit 不起作用
 * 页目录项中的 R/W bit 对其所映射的所有页面起作用
 */
#define PAGE_WRITE  0x2

/**
 * U/S bit 是 用户/超级用户 的标志
 * =1 表示运行在级别上的程序都可以访问该页面；=0 表示页面只能被运行在特权级上的程序访问
 * 页目录项的 U/S bit 对所映射的所有页面起作用
 */
#define PAGE_USER 0x4

#define PAGE_SIZE 4096        /* 分页大小 */
#define PAGE_MASK 0xfffff000  /* 页掩码，用于 4KB 对齐 */

/* 32bit = 10 + 10 + 12 */
#define PGD_INDEX(x) (((x) >> 22) & 0x3ff)  /* 获取一个地址的页目录项 */
#define PTE_INDEX(x) (((x) >> 12) & 0x3ff)  /* 获取一个地址的页表项 */
#define OFFSET_INDEX(x) ((x) & 0xfff)       /* 获取一个地址的页内便宜 */

typedef uint32_t pgd_t;
typedef uint32_t pte_t;

#define PGD_SIZE (PAGE_SIZE / sizeof(pte_t))      /* 页表成员数 */
#define PTE_SIZE (PAGE_SIZE / sizeof(uint32_t))   /* 页表成员数 */

#define PTE_COUNT 128 /* 映射 512MB 内存所需要的页表数，128 * (4096 / 4) * 4KB = 512MB */

extern pgd_t pgd_kern[PGD_SIZE];  /* 内核页目录区域 */

void init_vmm();                /* 初始化虚拟内存管理 */
void switch_pgd();              /* 更换当前页目录 */
void page_fault(pt_regs *regs); /* 页错误中断的函数处理 */

/* 使用 flags 指出页权限，把物理地址 pa 映射到虚拟地址 va */
void map(pgd_t *pgd_now, uint32_t va, uint32_t pa, uint32_t flags);
/* 取消地址映射 */
void unmap(pgd_t *pgd_now, uint32_t va);
/* 如果虚拟地址 va 映射到物理地址则返回 1，同时如果 pa 不是空指针则把物理地址写入 pa 参数 */
uint32_t get_mapping(pgd_t *pgd_now, uint32_t va, uint32_t *pa);

#endif