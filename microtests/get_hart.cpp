#include "utils.h"

int main(int argc, char** argv) {
  set_debug(get_hart() == 0);
  return 0;
}
