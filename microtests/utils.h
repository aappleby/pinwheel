#pragma once

inline int get_hart() {
  int dst;
  __asm__ volatile (
    "csrr %[dst], mhartid"
    : [dst] "=r" (dst)
  );
  return dst;
}

inline int csr_swap_secondary_thread(int dst) {
  __asm__ volatile (
    "csrrw %[dst], 0x800, %[dst]"
    : [dst] "+r" (dst)
  );
  return dst;
}

inline int csr_step_secondary_thread(int dst) {
  __asm__ volatile (
    R"(
      csrrw %[dst], 0x800, %[dst]
      csrrw %[dst], 0x800, %[dst]
    )"
    : [dst] "+r" (dst)
  );
  return dst;
}

inline int get_debug() {
  return *(volatile int*)(0xFFFFFFF0);
}

inline void set_debug(int x) {
  *(volatile int*)(0xFFFFFFF0) = x;
}

inline void test_fail() {
  set_debug(-1);
  while(1);
}

inline void test_pass() {
  set_debug(1);
  while(1);
}
