#ifndef INCLUDE_MUTILBOOT_H_
#define INCLUDE_MUTILBOOT_H_

#include "types.h"

/* 存储了GRUB在调用内核前获取的硬件信息和内核文件本身的一些信息 */
struct mutilboot {
  uint32_t flags; // mutilboot 的版本信息
  /**
   * 从 BIOS 获知的可用内存
   *
   * mem_lower 和 mem_upper 指出了低端和高端内存的大小
   * 低端内存首地址是 0，低端内存最大可能值是 640K
   * 高端内存首地址是 1M，高端内存最大可能值是 最大值减去 1M
   */
  uint32_t mem_lower;
  uint32_t mem_upper;

  uint32_t boot_device; // 指出引导程序从哪个磁盘设备载入
  uint32_t cmdline;     // 内核命令行
  uint32_t mods_count;  // boot 模块列表
  uint32_t mods_addr;

  /**
   * ELF 格式内核映像文件的 section 表
   */
  uint32_t num;   // section 数目
  uint32_t size;  // section 大小
  uint32_t addr;  // 段表地址
  uint32_t shndx; // 段表字符串表的索引

  /**
   * BIOS 提供的内存分布缓冲区地址和长度
   * mmap_addr 是缓冲区的地址，mmap_length 是缓冲区的大小
   * 缓冲区是 mmap_entry_t 组成的数组
   */
  uint32_t mmap_length;
  uint32_t mmap_addr;

  uint32_t drivers_length;   // 第一个驱动器结构的大小
  uint32_t drivers_addr;     // 第一个驱动器结构的物理地址
  uint32_t config_table;     // ROM 配置表
  uint32_t boot_loader_name; // boot loader 的名字
  uint32_t apm_table;        // APM 表

  uint32_t vbe_control_info;
  uint32_t vbe_mode_info;
  uint32_t vbe_mode;
  uint32_t vbe_interface_seg;
  uint32_t vbe_interface_off;
  uint32_t vbe_interface_len;
} __attribute__((packed));
typedef struct mutilboot mutilboot_t;

/* GRUB 探测到的内存结果按每个分段整理为 mmap_entry 结构体的数组 */
struct mmap_entry {
  uint32_t size;           // 除去 size 本身的大小
  uint32_t base_addr_low;  // 缓冲区地址的低 32bit
  uint32_t base_addr_high; // 缓冲区地址的高 32bit
  uint32_t length_low;     // 缓冲区大小的低 32bit
  uint32_t length_high;    // 缓冲区大小的高 32bit
  uint32_t type;           // 地址区间类型，1 代表可用，其它值代表保留区域
} __attribute__((packed));
typedef struct mmap_entry mmap_entry_t;

extern mutilboot_t *mboot_ptr_tmp;  // 内核未建立分页机制前暂存的指针
extern mutilboot_t *glb_mboot_ptr;  // 定义在 boot.s，分页机制建立之后的指针
#endif // INCLUDE_MUTILBOOT_H_