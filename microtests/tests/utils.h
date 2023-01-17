#pragma once

struct RV32Regfile {
  int zero; // x0
  int ra;   // x1
  int sp;   // x2
  int gp;   // x3
  int tp;   // x4
  int t0;   // x5
  int t1;   // x6
  int t2;   // x7

  int s0;   // x8
  int s1;   // x9
  int a0;   // x10
  int a1;   // x11
  int a2;   // x12
  int a3;   // x13
  int a4;   // x14
  int a5;   // x15

  int a6;   // x16
  int a7;   // x17
  int s2;   // x18
  int s3;   // x19
  int s4;   // x20
  int s5;   // x21
  int s6;   // x22
  int s7;   // x23

  int s8;   // x24
  int s9;   // x25
  int s10;  // x26
  int s11;  // x27
  int t3;   // x28
  int t4;   // x29
  int t5;   // x30
  int t6;   // x31
};

RV32Regfile* const regfiles = (RV32Regfile*)0x40000000;

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

inline int csr_yield_thread(int dst) {
  __asm__ volatile (
    "csrrw %[dst], 0x801, %[dst]"
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

__attribute__((naked))
inline void start_thread(int hpc) {
  __asm__ volatile (R"(
    la gp, __global_pointer$
    la sp, _stack_top
    csrr t0, mhartid
    /* 1024 bytes stack per hart*/
    sll t0, t0, 10
    sub sp, sp, t0
    j a0
  )");
}
