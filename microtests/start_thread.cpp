#include "utils.h"

extern void* _start;

int main(int argc, char** argv) {
  int hart = get_hart();

  if (hart == 0) {
    int hart1_entry = 0x01000000 | int(&_start);
    csr_swap_secondary_thread(hart1_entry);
    while(1);
  }
  else {
    test_pass();
  }

  return 0;
}
