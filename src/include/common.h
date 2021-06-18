/* 端口读写函数 */

#ifndef INCLUDE_COMMON_H_
#define INCLUDE_COMMON_H_

#include "types.h"

/* 端口写一个字节 */
void outb(uint16_t, uint8_t);

/* 端口读一个字节 */
uint8_t inb(uint16_t);

/* 端口读一个字 */
uint16_t inw(uint16_t);

/* 开启中断 */
void enable_intr();
/* 关闭中断 */
void disable_intr();

#endif  // INCLUDE_COMMON_H_