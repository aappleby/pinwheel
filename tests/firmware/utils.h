#pragma once

typedef unsigned long uint32_t;
typedef unsigned char uint8_t;

struct RV32Regfile {
  uint32_t zero; // x0
  uint32_t ra;   // x1
  uint32_t sp;   // x2
  uint32_t gp;   // x3
  uint32_t tp;   // x4
  uint32_t t0;   // x5
  uint32_t t1;   // x6
  uint32_t t2;   // x7

  uint32_t s0;   // x8
  uint32_t s1;   // x9
  uint32_t a0;   // x10
  uint32_t a1;   // x11
  uint32_t a2;   // x12
  uint32_t a3;   // x13
  uint32_t a4;   // x14
  uint32_t a5;   // x15

  uint32_t a6;   // x16
  uint32_t a7;   // x17
  uint32_t s2;   // x18
  uint32_t s3;   // x19
  uint32_t s4;   // x20
  uint32_t s5;   // x21
  uint32_t s6;   // x22
  uint32_t s7;   // x23

  uint32_t s8;   // x24
  uint32_t s9;   // x25
  uint32_t s10;  // x26
  uint32_t s11;  // x27
  uint32_t t3;   // x28
  uint32_t t4;   // x29
  uint32_t t5;   // x30
  uint32_t t6;   // x31
};

RV32Regfile* const regfiles = (RV32Regfile*)0x40000000;

inline uint32_t get_hart() {
  uint32_t dst;
  __asm__ volatile (
    "csrr %[dst], mhartid"
    : [dst] "=r" (dst)
  );
  return dst;
}

inline uint32_t csr_swap_secondary_thread(uint32_t dst) {
  __asm__ volatile (
    "csrrw %[dst], 0x800, %[dst]"
    : [dst] "+r" (dst)
  );
  return dst;
}

inline uint32_t pinwheel_start(int hart, void (*pc)()) {
  uint32_t dst = ((hart & 0xFF) << 24) | (int(pc) & 0xFFFFFF);
  return csr_swap_secondary_thread(dst);
}

inline uint32_t pinwheel_stop() {
  return csr_swap_secondary_thread(0);
}

inline uint32_t csr_yield_thread(uint32_t dst) {
  __asm__ volatile (
    "csrrw %[dst], 0x801, %[dst]"
    : [dst] "+r" (dst)
  );
  return dst;
}

inline uint32_t csr_step_secondary_thread(uint32_t dst) {
  __asm__ volatile (
    R"(
      csrrw %[dst], 0x800, %[dst]
      csrrw %[dst], 0x800, %[dst]
    )"
    : [dst] "+r" (dst)
  );
  return dst;
}

inline uint32_t get_debug() {
  return *(volatile uint32_t*)(0xFFFFFFF0);
}

inline void set_debug(uint32_t x) {
  *(volatile uint32_t*)(0xFFFFFFF0) = x;
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
void* get_sp() {
  __asm__(R"(
    mv a0, sp
    ret
  )");
}

__attribute__((naked))
void* get_gp() {
  __asm__(R"(
    mv a0, gp
    ret
  )");
}
