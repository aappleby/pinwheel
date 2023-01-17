#include "utils.h"

extern void* _start;

// 3 instructions to set the debug register, so 3 steps should end the test.
__attribute__((naked))
void hart1_start() {
  __asm__(R"(
    li t0, 0xFFFFFFF0
    csrr t1, mhartid
    sw t1, 0(t0)
    ret
  )");
}

int main(int argc, char** argv) {
  int hart1_hpc = 0x01000000 | int(&hart1_start);

  if (get_debug() != 0) test_fail();
  hart1_hpc = csr_step_secondary_thread(hart1_hpc);
  if (get_debug() != 0) test_fail();
  hart1_hpc = csr_step_secondary_thread(hart1_hpc);
  if (get_debug() != 0) test_fail();
  hart1_hpc = csr_step_secondary_thread(hart1_hpc);

  // Should not get here, as hart 1 writes the debug register and ends the test.
  test_fail();

  return 0;
}
