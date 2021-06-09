/* 屏幕相关操作 */

#ifndef INCLUDE_CONSOLE_H_
#define INCLUDE_CONSOLE_H_

#include "types.h"

enum real_color {
  rc_back = 0,
  rc_blue = 1,
  rc_green = 2,
  rc_cyan = 3,
  rc_red = 4,
  rc_magenta = 5,
  rc_brown = 6,
  rc_light_grey = 7,
  rc_dark_grey = 8,
  rc_light_blue = 9,
  rc_light_green = 10,
  rc_light_cyan = 11,
  rc_light_red = 12,
  rc_light_magenta = 13,
  rc_light_brown = 14, // yellow
  rc_white = 15
};
typedef enum real_color real_color_t;

/* 清屏操作 */
void console_clear();

/* 屏幕输出一个带颜色的字符 */
void console_putc_color(char c, real_color_t back, real_color_t fore);

/* 打印字符串 */
void console_write(char *cstr);

/* 打印带颜色的字符串 */
void console_write_color(char *cstr, real_color_t back, real_color_t fore);

void console_write_hex(uint32_t n, real_color_t back, real_color_t fore);
void console_write_dec(uint32_t n, real_color_t back, real_color_t fore);

#endif // INCLUDE_CONSOLE_H_