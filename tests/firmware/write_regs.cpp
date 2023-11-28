#include "utils.h"

// Write hart 1's registers from hart 0, then yield to hart 1 and verify
// that the written value is in the correct register.

extern "C" __attribute__((naked))
void test_write_regs_dst() {
  __asm__ volatile (R"(
      li t0, 0xF00DCAFE
      bne t0, x23, _fail

    _pass:
      li t0, 1
      li t1, 0xFFFFFFF0
      sw t0, 0(t1)
    _fail:
      li t0, -1
      li t1, 0xFFFFFFF0
      sw t0, 0(t1)
    end:
      j end
  )");
}

int main(int argc, char** argv) {
  int* regs = (int*)0xE0000000;
  regs[32 + 23] = 0xF00DCAFE;
  csr_yield_thread((int)test_write_regs_dst | 0x81000000);
  while(1);
  return 0;
}
