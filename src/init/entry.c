#include "console.h"
#include "debug.h"

int kern_entry() {
  console_clear();
  init_debug();
  console_write_color("Hello, OS kernel!\n", rc_back, rc_green);
  panic("func trace");
  return 0;
}