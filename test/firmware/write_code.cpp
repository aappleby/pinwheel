#include "utils.h"

unsigned int write_code_good[4] = {
  0x00100293, // addi r05, r00, 1
  0xff000313, // addi r06, r00, -16
  0x00532023, // sw r05, [r06+0]
  0x0000006F, // jal r00, [+0]
};

extern "C" __attribute__((naked))
void write_code_bad() {
  __asm__ volatile (R"(
      li t0, -1
      li t1, 0xFFFFFFF0
      sw t0, 0(t1)
      1: j 1b
  )");
}

int main(int argc, char** argv) {
  volatile char* src = (volatile char*)write_code_good;
  volatile char* dst = (volatile char*)&write_code_bad;

  for (int i = 0; i < 16; i++) {
    dst[i] = src[i];
  }

  write_code_bad();

  return 0;
}
