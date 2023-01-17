#include "utils.h"

extern void* _start;

extern "C" __attribute__((naked))
void test_jalr_src() {
  __asm__ volatile (R"(
    la t0, test_jalr_dst
    li t1, 0x01000000
    or t0, t0, t1
    jalr t0
  )");
}

extern "C" __attribute__((naked))
void test_jalr_dst() {
  __asm__ volatile (R"(
      beqz ra, no_r1
      li t0, 1
      li t1, 0xFFFFFFF0
      sw t0, 0(t1)
    no_r1:
      li t0, -1
      li t1, 0xFFFFFFF0
      sw t0, 0(t1)
    end:
      j end
  )");
}

int main(int argc, char** argv) {
  test_jalr_src();
  return 0;
}
