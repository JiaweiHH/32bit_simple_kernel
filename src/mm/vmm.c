#include "vmm.h"
#include "debug.h"
#include "idt.h"
#include "mstring.h"
#include "pmm.h"

/**
 * 注意，开启分页之后代码操作的全部都是虚拟地址，
 * 开启分页之前理论上也是虚拟地址，但是那个时候虚拟地址和物理地址一一对应
 */

/* 内核页目录，里面保存的页表的物理地址 */
pgd_t pgd_kern[PGD_SIZE] __attribute__((aligned(PAGE_SIZE)));
/* 内核页表，里面保存的是物理页框 */
static pte_t pte_kern[PTE_COUNT][PTE_SIZE] __attribute__((aligned(PAGE_SIZE)));

/* 虚拟内存初始化，主要是建立 pgd 和 pte 的映射关系，pte 和物理页框的关系 */
void init_vmm() {
  /* 0xC0000000 这个地址在页目录的索引 */
  uint32_t kern_pte_first_idx = PGD_INDEX(PAGE_OFFSET);

  /* 初始化 pgd 的表项，使其内容为对应的 pte 页表地址 */
  uint32_t i, j;
  for (i = kern_pte_first_idx, j = 0; i < PTE_COUNT + kern_pte_first_idx;
       ++i, ++j) {
    pgd_kern[i] =
        ((uint32_t)pte_kern[j] - PAGE_OFFSET) | PAGE_PRESENT | PAGE_WRITE;
  }

  /* 初始化 pte 表项，其内容为物理页框；不映射第 0 页，用于跟踪 null 指针 */
  uint32_t *pte = (uint32_t *)pte_kern;
  for (i = 1; i < PTE_COUNT * PTE_SIZE; ++i) {
    pte[i] = (i << 12) | PAGE_PRESENT | PAGE_WRITE;
  }

  /* pgd 页目录的物理地址 */
  uint32_t pgd_kern_phy_addr = (uint32_t)pgd_kern - PAGE_OFFSET;

  /* 注册缺页中断 */
  register_interrupt_handler(14, &page_fault);

  /* 切换页表，可以不需要临时页表了 */
  switch_pgd(pgd_kern_phy_addr);
}

void switch_pgd(uint32_t pd) { asm volatile("mov %0, %%cr3" : : "r"(pd)); }

/* 建立 va 和 pa 的映射关系，必要时分配页表 */
void map(pgd_t *pgd_now, uint32_t va, uint32_t pa, uint32_t flags) {
  uint32_t pgd_idx = PGD_INDEX(va);
  uint32_t pte_idx = PTE_INDEX(va);

  pte_t *pte = (pte_t *)(pgd_now[pgd_idx] & PAGE_MASK);

  /* 找到 pgd 中对应的表项，返回它的内核地址，因为开启了分页 cpu 使用的都是虚拟地址了 */
  if (!pte) { /* 如果此映射还不存在页表，那么分配一页作为页表 */
    pte = (pte_t *)pmm_alloc_page();
    /* 设置页目录对应的表项为该新分配的页表物理地址 */
    pgd_now[pgd_idx] = (uint32_t)pte | PAGE_PRESENT | PAGE_WRITE;
    
    /* 转为内核地址并清 0 */
    pte = (pte_t *)((uint32_t)pte + PAGE_OFFSET);
    bzero(pte, PAGE_SIZE);
  } else {  /* 此映射的页表一开始就存在，将其物理地址转换成内核地址 */
    pte = (pte_t *)((uint32_t)pte + PAGE_OFFSET);
  }

  /* 设置页表的表项为 pa 对应的物理页框 */
  pte[pte_idx] = (pa & PAGE_MASK) | flags;
  /* 通知 cpu 更新页表缓存 */
  asm volatile ("invlpg (%0)" : : "a"(va));
}

void unmap(pgd_t *pgd_now, uint32_t va) {
  uint32_t pgd_idx = PGD_INDEX(va);
  uint32_t pte_idx = PTE_INDEX(va);

  /* 找到页目录的表项 */
  pte_t *pte = (pte_t *)(pgd_now[pgd_idx] & PAGE_MASK);
  if(!pte)
    return;
  /* 找到页表的表项，设置为 0 */
  pte = (pte_t *)((uint32_t)pte + PAGE_OFFSET);
  pte[pte_idx] = 0;
  asm volatile ("invlpg (%0)" : : "a"(va));
}

uint32_t get_mapping(pgd_t *pgd_now, uint32_t va, uint32_t *pa) {
  uint32_t pgd_idx = PGD_INDEX(va);
  uint32_t pte_idx = PTE_INDEX(va);
  
  pte_t *pte = (pte_t *)(pgd_now[pgd_idx] & PAGE_MASK);
  if(!pte)
    return;

  pte = (pte_t *)((uint32_t)pte + PAGE_OFFSET);

  /* 如果地址有效，且传入的保存物理地址的指针不为 null */
  if(pte[pte_idx] != 0 && pa) {
    *pa = pte[pte_idx] & PAGE_MASK;
    return 1;
  }
  return 0;
}