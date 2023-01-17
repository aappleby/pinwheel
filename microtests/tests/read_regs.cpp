#include "utils.h"

// Write hart 1's registers from hart 0, then read them back.

int main(int argc, char** argv) {
  volatile int* regs = (volatile int*)0xE0000000;
  regs[32 + 23] = 0xF00DCAFE;

  if (regs[32+23] != 0xF00DCAFE) {
    test_fail();
  }
  else {
    test_pass();
  }

  return 0;
}
