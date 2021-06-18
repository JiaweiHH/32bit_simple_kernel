#include "timer.h"
#include "common.h"
#include "debug.h"
#include "idt.h"
#include "msched.h"

#define HZ 1193180

void timer_callback(pt_regs *regs) {
  schedule();
}

/**
 * 时钟中断由 8253 实现
 * 时钟中断的发生频率是 frequency = 1193180/计数器的初始值，因此，计数器的初始值 = 1193180/frequency
 * https://juejin.cn/post/6938051055733178399
 */
void init_timer(uint32_t frequency) {
  /* 注册 timer 中断处理函数 */
  register_interrupt_handler(IRQ0, timer_callback);

  /* 计数器初始值，时钟中断的频率是 HZ / divisor */
  uint32_t divisor = HZ / frequency;

  /* 先设置 0x43 端口，0x43 端口占 8bit，每个 bit 有特殊意义，总之就是按照要求设置 */
  outb(0x43, 0x36);
  uint8_t low = (uint8_t)(divisor & 0xff);
  uint8_t high = (uint8_t)((divisor >> 8) & 0xff);
  /* 0x40 端口设置计数器的值，需要分两次进行，先写低字节再写高字节 */
  outb(0x40, low);
  outb(0x40, high);
}