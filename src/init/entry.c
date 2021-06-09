#include "console.h"

int kern_entry() {
  console_clear();
  console_write_color("Hello, OS kernel!\n", rc_back, rc_green);
  return 0;
}