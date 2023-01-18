#include "utils.h"

extern void* _start;

int main(int argc, char** argv) {
  int hart = get_hart();

  if (hart == 0) {
    int hart1_entry = 0x01000000 | int(&_start);
    csr_yield_thread(hart1_entry);
    test_fail();
  }
  else {
    volatile int i = 0;
    while(i < 10) {
      i = i + 1;
    }
    test_pass();
  }

  return 0;
}
