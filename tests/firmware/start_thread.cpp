#include "utils.h"

extern void* _start;

int main(int argc, char** argv) {
  int hart = get_hart();

  if (hart == 0) {
    uint32_t hart1_entry = (1 << 24) | uint32_t(&_start);
    csr_swap_secondary_thread(hart1_entry);
    while(1);
  }
  else {
    test_pass();
  }

  return 0;
}
