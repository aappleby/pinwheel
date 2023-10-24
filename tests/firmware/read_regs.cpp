#include "utils.h"

// Write hart 1's registers from hart 0, then read them back.

extern "C" __attribute__((naked))
void busy_wait() {
  __asm__ volatile (R"(
    end:
      j end
  )");
}


int main(int argc, char** argv) {
  volatile uint32_t* regs = (volatile uint32_t*)0xE0000000;

  for (int i  = 32; i < 64; i++) {
    pinwheel_stop();
    regs[i] = 0xF00DCAFE;
    if (regs[i] != 0xF00DCAFE) test_fail();

    // Start busy waiting on hart 2 not 1, otherwise we see spurious copies
    // of 0xF00DCAFE on the bus from hart 1 fetching r0 during jal
    pinwheel_start(2, busy_wait);
    if (regs[i] == 0xF00DCAFE) test_fail();

    pinwheel_stop();
    if (regs[i] != 0xF00DCAFE) test_fail();
  }

  test_pass();

  return 0;
}
