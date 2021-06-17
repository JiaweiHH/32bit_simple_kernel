#include "heap.h"
#include "debug.h"
#include "pmm.h"
#include "vmm.h"

static void alloc_chunk(uint32_t start, uint32_t len);  /* 申请内存块 */
static void free_chunk(header_t *chunk);                /* 释放内存块 */
static void split_chunk(header_t *chunk, uint32_t len);      /* 切分内存块 */
static void glue_chunk(header_t *chunk);                /* 合并内存块 */

static uint32_t heap_max = HEAP_START;  /* 标记已分配的内存的最后，不包括 heap_max */
static header_t *heap_first;  /* 内存块管理头指针 */

void init_heap() { heap_first = 0; }

/**
 * 在 Linux 中 kmalloc 和 vmalloc 都是在申请内存的时候直接分配物理内存的
 * kmalloc 分配得到的物理内存是连续的，vmalloc 分配得到的物理内存不是连续的
 * 而用户态的 malloc 只是先申请虚拟内存，并不实际分配物理内存
 */
void *kmalloc(uint32_t len) {
  /* 一个 chunk 里面包含了一个管理结构和可用内存 */
  len += sizeof(header_t);

  header_t *cur_header = heap_first;
  header_t *prev_header = 0;

  while(cur_header) {
    /* 如果当前内存块没有被申请过，而且长度大于被申请的 */
    if(cur_header->allocated == 0 && cur_header->length >= len) {
      /* 按照当前长度切割内存 */
      split_chunk(cur_header, len);
      cur_header->allocated = 1;
      /* 跳过 chunk 里面的管理结构 */
      return (void *)((uint32_t)cur_header + sizeof(header_t));
    }
    prev_header = cur_header;
    cur_header = cur_header->next;
  }

  uint32_t chunk_start;

  /* 如果没有可以分配的，那么分配一个新的 内存块管理 */
  if(prev_header) { /* 不是第一次分配 */
    chunk_start = (uint32_t)prev_header + prev_header->length;
  } else {  /* 第一次分配 */
    chunk_start = HEAP_START;
    heap_first = (header_t *)chunk_start;
  }

  /**
   * 分配物理内存，一次只能分配一个 page
   * 如果判断发现当前堆内存上次分配的 page 还有空闲空间可以容纳 len，
   * 则不分配直接设置 cur_header 结构体
   */
  alloc_chunk(chunk_start, len);
  cur_header = (header_t *)chunk_start;
  cur_header->prev = prev_header;
  cur_header->next = 0;
  cur_header->allocated = 1;
  cur_header->length = len;
  if(prev_header)
    prev_header->next = cur_header;

  /* 跳过 chunk 里面的管理结构 */
  return (void *)(chunk_start + sizeof(header_t));
}

void kfree(void *p) {
  /* 指针回退到管理结构，并将已使用标记置为 0 */
  header_t *header = (header_t *)((uint32_t)p - sizeof(header_t));
  header->allocated = 0;

  /* 尝试合并内存块 */
  glue_chunk(header);
}

void alloc_chunk(uint32_t start, uint32_t len) {
  /**
   * 循环分配，直到新分配的物理内存可以容纳 len 的内存
   * 当前堆的位置已经达到界限，则申请内存页；
   * 只能一页一页分配，不足一页也分配一页
   */
  while(start + len > heap_max) {
    uint32_t page = pmm_alloc_page();
    map(pgd_kern, heap_max, page, PAGE_PRESENT | PAGE_WRITE);
    heap_max += PAGE_SIZE;
  }
}

void free_chunk(header_t *chunk) {
  if(chunk->prev == 0) {  /* chunk 是最后一个内存管理块 */
    heap_first = 0;
  } else {
    chunk->prev->next = 0;
  }

  /**
   * 待释放的 chunk 所占的大小超过一个 page 才释放物理内存，
   * 可能会出现一个 page 里面有多个 chunk，这样一来该 page 还会被使用，所以就不能释放
   */
  while((heap_max - PAGE_SIZE) >= (uint32_t)chunk) {
    heap_max -= PAGE_SIZE;
    uint32_t page;
    get_mapping(pgd_kern, heap_max, &page);
    unmap(pgd_kern, heap_max);
    pmm_free_page(page);
  }
}

void split_chunk(header_t *chunk, uint32_t len) {
  /* 切分内存块之前保证之后的剩余内容至少容纳一个管理内存块的大小 */
  if(chunk->length - len > sizeof(header_t)) {
    /* 切分一个 chunk 出来，并连接原先的 chunk 以及其 next 部分，剩余的一个 old_chunk 作为分配使用 */
    header_t *new_chunk = (header_t *)((uint32_t)chunk + len);
    new_chunk->prev = chunk;
    new_chunk->next = chunk->next;
    new_chunk->allocated = 0;
    new_chunk->length = chunk->length - len;

    /* old_chunk 作为分配使用 */
    chunk->next = new_chunk;
    chunk->length = len;
  }
}

void glue_chunk(header_t *chunk) {
  /* 如果该内存块前后有连接内存块且未被使用，则合并 */
  if(chunk->next && chunk->next->allocated == 0) {
    chunk->length += chunk->next->length;
    if(chunk->next->next)
      chunk->next->next->prev = chunk;
    chunk->next = chunk->next->next;
  }
  if(chunk->prev && chunk->prev->allocated == 0) {
    chunk->prev->length += chunk->length;
    chunk->prev->next = chunk->next;
    if(chunk->next)
      chunk->next->prev = chunk->prev;
    chunk = chunk->prev;
  }

  /* 假如内存后面没有连接内存块了，直接释放 */
  if(chunk->next == 0)
    free_chunk(chunk);
}

void test_heap() {
  printk_color(rc_black, rc_magenta, "Test kmalloc() && kfree() now ...\n\n");
  
  void *addr1 = kmalloc(50);
  printk("kmalloc     50 bytes in 0x%X\n", addr1);
  void *addr2 = kmalloc(500);
  printk("kmalloc    500 bytes in 0x%X\n", addr2);
  void *addr3 = kmalloc(5000);
  printk("kmalloc   5000 bytes in 0x%X\n", addr3);
  void *addr4 = kmalloc(50000);
  printk("kmalloc  50000 bytes in 0x%X\n", addr4);

  kfree(addr1);
  printk("free mem in 0x%X\n", addr1);
  kfree(addr2);
  printk("free mem in 0x%X\n", addr2);
  kfree(addr3);
  printk("free mem in 0x%X\n", addr3);
  kfree(addr4);
  printk("free mem in 0x%X\n", addr4);
}