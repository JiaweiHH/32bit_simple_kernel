#include "console.h"
#include "debug.h"
#include "gdt.h"

int kern_entry() {
  init_debug();
  init_gdt();
  console_clear();
  console_write_color("Hello, OS kernel!\n", rc_back, rc_green);
  return 0;
}