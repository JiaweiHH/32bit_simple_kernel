#include "melf.h"
#include "common.h"
#include "mstring.h"
#include "vmm.h"

/* 在 mutilboot 提供的信息中寻找字符串表和符号表 */
elf_t elf_from_mutilboot(mutilboot_t *mb) {
  int i;
  elf_t elf;
  elf_section_header_t *sh = (elf_section_header_t *)mb->addr; // 获取段表地址
  uint32_t shstrtab = sh[mb->shndx].addr; // 获取段表字符串表地址
  for (i = 0; i < mb->num; ++i) {         // 寻找字符串表和符号表
    const char *name =
        (const char *)(shstrtab + sh[i].name + PAGE_OFFSET); // 查找段表字符串表找到段的名字
    /* 判断段的名字是不是我们需要寻找的字符串表和符号表 */
    if (strcmp(name, ".strtab") == 0) {
      elf.strtab = (const char *)(sh[i].addr + PAGE_OFFSET);
      elf.strtabsz = sh[i].size;
    } else if (strcmp(name, ".symtab") == 0) {
      elf.symtab = (elf_symbol_t *)(sh[i].addr + PAGE_OFFSET);
      elf.symtabsz = sh[i].size;
    }
  }
  return elf;
}

/* 查找 addr 地址对应的函数名字 */
const char *elf_lookup_symbol(uint32_t addr, elf_t *elf) {
  int i;
  for (i = 0; i < (elf->symtabsz / sizeof(elf_symbol_t)); ++i) {
    if (ELF32_ST_TYPE(elf->symtab[i].info) != 0x2)  // 0x2 的符号类型是函数
      continue;
    // 判断 addr 是不是落在 symtab[i] 这个函数的范围区间里面
    if ((addr >= elf->symtab[i].value) &&
        (addr < (elf->symtab[i].value + elf->symtab[i].size))) {
      return (const char *)((uint32_t)elf->strtab + elf->symtab[i].name);
    }
      
  }
  return NULL;
}