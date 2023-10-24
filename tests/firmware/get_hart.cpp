#include "utils.h"

// Make sure get_hart works

int main(int argc, char** argv) {
  set_debug(get_hart() == 0);
  return 0;
}
