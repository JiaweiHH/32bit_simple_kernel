#ifndef INCLUDE_ELF_H_
#define INCLUDE_ELF_H_

#include "mutilboot.h"
#include "types.h"

#define ELF32_ST_TYPE(i) ((i)&0xf)  /* 获取符号类型 */

/* 段描述符 */
struct elf_section_header {
  uint32_t name;        /* 在 shstrtab 段表字符串表中的下标 */
  uint32_t type;        /* 段的类型 */
  uint32_t flags;
  uint32_t addr;
  uint32_t offset;      /* 该段在文件中的偏移 */
  uint32_t size;        /* 段长度 */
  uint32_t link;
  uint32_t info;
  uint32_t addralign;
  uint32_t entsize;
} __attribute__((packed));
typedef struct elf_section_header elf_section_header_t;

/* 符号表元素 */
struct elf_symbol {
  uint32_t name;    /* 符号名在字符串表中的下标 */
  uint32_t value;   /* 符号的地址或偏移 */
  uint32_t size;    /* 符号的大小 */
  uint8_t info;     /* 低 4 位表示符号类型，高 4bit 是绑定信息
                       符号类型：段名、数据对象等
                       符号绑定信息：全局符号、局部符号等 */
  uint8_t other;    /* 不使用 */
  uint16_t shndx;   /* 符号所在的段 */
} __attribute__((packed));
typedef struct elf_symbol elf_symbol_t;

/* elf 整体信息 */
struct elf {
  elf_symbol_t *symtab;   /* 符号表地址 */
  uint32_t symtabsz;      /* 符号表大小，单位是字节 */
  const char *strtab;     /* 字符串表地址 */
  uint32_t strtabsz;      /* 字符串表大小，单位是字节 */
};
typedef struct elf elf_t;

/* 在 mutilboot 提供的信息中寻找字符串表和符号表 */
elf_t elf_from_mutilboot(mutilboot_t *mb);

/* 查找 addr 地址对应的函数名字 */
const char *elf_lookup_symbol(uint32_t addr, elf_t *elf);

#endif