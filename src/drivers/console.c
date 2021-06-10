#include "console.h"
#include "common.h"

/* 屏幕被分成 25*80 的二维数组 */
/**
   * 地址空间的最低 1MB 很多地址都是属于外部设备的
   * 0xB8000 ~ 0xC0000 是显卡工作在文本模式下的显存
   * 
   * 从 0xB8000 ~ 0xC0000 每 2 个字节对应屏幕上的一个字符，
   * 第一个字节是字符的 ASCII 码，第二个字节是字符颜色控制信息
   * Bit:     | 15 14 13 12 11 10 9 8 | 7 6 5 4 | 3 2 1 0 |
   * Content: | ASCII                 | FG      | BG      |
   */

static uint16_t *video_memory = (uint16_t *)0xB8000;  // 地址空间显示显卡字符的起始地址是 0xB8000
static uint8_t cursor_x = 0, cursor_y = 0;  // 屏幕光标的坐标，x 是横坐标，y 是纵坐标

static void move_cursor() {
  uint16_t cursor_location = cursor_y * 80 + cursor_x;
  /**
   * 显卡除了显示内容的存储单元之外，还有其他的显示控制单元，
   * 这些控制单元编址在独立的 I/O 空间，需要使用 in/out 指令去读写。
   * 相关的控制寄存器很多，无法映射到 I/O 端口的地址空间，
   * 因此解决方法是将 0x3D4 作为内部寄存器的索引，再通过 0x3D5 端口设置相应的寄存器
   */ 
  // 这里的端口和值都是固定的，显卡 VGA 标准规定
  outb(0x3D4, 14);                    // 14 表示设置高 8bit
  outb(0x3D5, cursor_location >> 8);
  outb(0x3D4, 15);                    // 15 表示设置低 8bit
  outb(0x3D5, cursor_location);
}

void console_clear() {
  uint8_t attribute_byte = (0 << 4) | (15 & 0x0F);  // 黑底白字的颜色属性
  uint16_t blank = 0x20 | (attribute_byte << 8);    // 上述属性的空格
  int i;
  for(i = 0; i < 80 * 25; ++i) {
    video_memory[i] = blank;
  }
  cursor_x = 0;
  cursor_y = 0;
  move_cursor();
}

static void scroll() {
  uint8_t attribute_byte = (0 << 4) | (15 & 0x0F);  // 黑底白字的颜色属性
  uint16_t blank = 0x20 | (attribute_byte << 8);
  if(cursor_y >= 25) {
    int i;
    for(i = 0; i < 24 * 80; i++) {
      video_memory[i] = video_memory[i + 80];
    }
    for(i = 24 * 80; i < 25 * 80; ++i) {
      video_memory[i] = blank;
    }
    cursor_y = 24;
  }
}

void console_putc_color(char c, real_color_t back, real_color_t fore) {
  uint8_t back_color = (uint8_t)back;
  uint8_t fore_color = (uint8_t)fore;

  uint8_t attribute_byte = (back_color << 4) | (fore_color & 0x0F);
  uint16_t attribute = attribute_byte << 8;

  if(c == 0x08 && cursor_x) {   // 退格键
    --cursor_x;
  } else if(c == 0x09) {        // tab 键
    cursor_x = (cursor_x + 8) & ~(8 - 1);
  } else if(c == '\r') {        // 回车符
    cursor_x = 0;
  } else if(c == '\n') {        // 换行符
    cursor_x = 0;
    ++cursor_y;
  } else if(c >= ' ') {
    video_memory[cursor_y * 80 + cursor_x] = c | attribute;
    ++cursor_x;
  }

  if(cursor_x >= 80) {
    cursor_x = 0;
    ++cursor_y;
  }

  scroll();
  move_cursor();
}

void console_write(char *cstr) {
  while(*cstr) {
    console_putc_color(*cstr++, rc_back, rc_white);
  }
}

void console_write_color(char *cstr, real_color_t back, real_color_t fore) {
  while(*cstr)
    console_putc_color(*cstr++, back, fore);
}