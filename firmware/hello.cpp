#include <stdarg.h>

// where is this?
//#include <rvintrin.h>

/*

gdb command -
-> $packet-data#checksum
<- + (or -)

https://sourceware.org/gdb/onlinedocs/gdb/Packets.html#Packets

*/

/*
x1      = ra     = return address
x2      = fp     = frame pointer
x3-x13  = s1-s11 = saved registers
x14     = sp     = stack pointer
x15     = tp     = thread pointer
x16-17  = v0-v1  = return values
x18-x25 = a0-a7  = function arguments
x26-x30 = t0-t4  = temporaries
x31     = gp     = global pointer
*/

/*
R type: .insn r opcode, func3, func7, rd, rs1, rs2
+-------+-----+-----+-------+----+-------------+
| func7 | rs2 | rs1 | func3 | rd |      opcode |
+-------+-----+-----+-------+----+-------------+
31      25    20    15      12   7             0
*/

typedef unsigned long uint32_t;
typedef unsigned char uint8_t;

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

__attribute__ ((naked)) int get_hart()
{
  asm(R"(
    csrr a0, mhartid
    ret
  )");
}

__attribute__((naked))
uint32_t start_hart(uint32_t hart, uint32_t address) {
  __asm__(R"(
    .insn r 0x0B, 0, 0, a0, a0, a1
    ret
  )");
}

__attribute__((naked))
uint32_t csrrw_800(uint32_t val, uint32_t address) {
  __asm__(R"(
    csrrw a0, 800, a0
    ret
  )");
}


// https://sourceware.org/binutils/docs-2.34/as/RISC_002dV_002dFormats.html

__attribute__((naked))
uint32_t step_hart(uint32_t hart, uint32_t address) {
  __asm__(R"(
    .insn r 0x0B, 0, 0, a0, a0, a1
    .insn r 0x0B, 0, 0, a0, a0, x0
    ret
  )");
}

__attribute__((naked))
uint32_t yield_hart(uint32_t hart, uint32_t address) {
  __asm__(R"(
    .insn r 0x0B, 1, 0, x0, a0, a1
    ret
  )");
}

//------------------------------------------------------------------------------

struct Console {
  void puts(const char* s) {
    while(*s) {
      *out = *s;
      s++;
    }
  }

  void putnx(uint8_t x) {
    *out = (x > 9 ? 'A' - 10 : '0') + x;
  }

  void putbx(uint8_t x) {
    putnx((x >> 4) & 0x0F);
    putnx((x >> 0) & 0x0F);
  }

  void putdx(uint32_t x) {
    for (int i = 28; i >= 0; i -= 4) {
      putnx((x >> i) & 0xF);
    }
  }

  void putx(uint32_t x) {
    if (x == 0) {
      *out = '0';
      return;
    }

    uint32_t t = x;
    int c = 0;
    while(t > 0xF) {
      t >>= 4;
      c += 4;
    }

    for (int i = c - 4; i >= 0; i -= 4) {
      putnx((x >> i) & 0xF);
    }
  }

  void putd(int d) {
    if (d == 0) {
      *out = '0';
      return;
    }

    if (d < 0) {
      *out = '-';
      d = -d;
    }

    int count = 0;
    uint8_t buf[16];

    while(d >= 10) {
      buf[count++] = (d % 10);
      d = d / 10;
    }

    buf[count++] = d;

    for (int i = count - 1; i >= 0; i--) {
      *out = '0' + buf[i];
    }
  }

  void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    while(*format) {
      if (*format == '\\') {
        format++;
        *out = *format++;
      }
      else if (*format == '%') {
        format++;
        switch(*format++) {
        case 'c': *out = (char)va_arg(args, int); break;
        case 'd': putd(va_arg(args, int)); break;
        case 'x': putx(va_arg(args, uint32_t)); break;
        case 'p': putdx(va_arg(args, uint32_t)); break;
        case 's': puts(va_arg(args, const char*)); break;
        }
      } else {
        *out = *format++;
      }
    }
  }

  volatile uint32_t* out;
};


//------------------------------------------------------------------------------

extern "C"
__attribute__((naked, __section__(".start")))
void _start() {
  __asm__(R"(
  .option push
  .option norelax
    la gp, __global_pointer$
    la sp, _stack_top
    csrr t0, mhartid
    /* 256 bytes stack per hart, which is rather tight... */
    sll t0, t0, 8
    sub sp, sp, t0
  main_loop:
    call main
    j main_loop
  .option pop
  )");
}

//------------------------------------------------------------------------------

Console c1 = { (uint32_t*)0x40000000 };
Console c2 = { (uint32_t*)0x50000000 };
Console c3 = { (uint32_t*)0x60000000 };
Console c4 = { (uint32_t*)0x70000000 };

//------------------------------------------------------------------------------

typedef int (*start_func)(void);

// Need to implement PC control via csr registers

inline uint32_t csr_swap_secondary_thread(uint32_t dst) {
  __asm__(
    "csrrw %[dst], 0x800, %[dst]"
    : [dst] "+r" (dst)
  );
  return dst;
}

int main(int argc, char** argv) {
  int hart = get_hart();

  Console* c = hart == 0 ? &c1 : &c2;

  do {
    c->printf("hart    %d\n",   hart);
    c->printf("main    0x%p\n", main);
    c->printf("stack   0x%p\n", get_sp());
    c->printf("global  0x%p\n", get_gp());

    c->printf("decimal 1234567890 %d\n", 1234567890);
    c->printf("decimal -123456789 %d\n", -123456789);
    c->printf("hex     0x12345678 0x%x\n", 0x12345678);
    c->printf("hex     0x1234     0x%x\n", 0x1234);
    c->printf("pointer 0x12345678 0x%p\n", 0x12345678);
    c->printf("pointer 0x00001234 0x%p\n", 0x00001234);
    c->printf("char    !@#$\\%^&*() %c%c%c%c%c%c%c%c%c%c\n", '!', '@', '#', '$', '%', '^', '&', '*', '(', ')');
  } while(hart == 1);

  if (hart == 0) {
    c->printf("Starting hart 1\n");
    uint32_t old_thread = csr_swap_secondary_thread(0x01040000);
    c->printf("old_thread 0x%p\n", old_thread);
  }
  while(1);

  /*
  if (hart == 0) {
    c1.printf("Forking to hart 1\n");
    start_func start0 = reinterpret_cast<start_func>(_start);
    start_func start1 = reinterpret_cast<start_func>(reinterpret_cast<uint32_t>(start0) + 0x01000000);
    c1.printf("start0   0x%p\n", start0);
    c1.printf("start1   0x%p\n", start1);
    start1();
  }
  */

  *(volatile uint32_t*)0xFFFFFFF0 = 1;
  c->printf("Test pass\n\n\n");
  return 0;
}

//------------------------------------------------------------------------------











  // test read from code mem... this can't work with the way pinwheel is set up
  /*
  for (int i = 0; i < 100; i++) {
    volatile uint32_t* code = (volatile uint32_t*)(i * 4);
    c1.printf("Code @ %p = %p\n", code, *code);
  }
  */

  // test write to code mem
  /*
  volatile uint32_t* code = (volatile uint32_t*)0x00000000;
  for(int i = 0; i < 1000000; i++) {
    code[i] = 0xDEADBEEF;
  }
  */








  //for (int i = 0; i < 300; i++) {
  /*
  while(1) {
    pc = step_hart(1, pc);
    //c1.printf("pc 0x%p\n", pc);
    volatile uint32_t* r = ((volatile uint32_t*)0xE0000000) + 32;
    c1.printf("r00 %p  r08 %p  r16 %p  r24 %p\n", r[ 0], r[ 8], r[16], r[24]);
    c1.printf("r01 %p  r09 %p  r17 %p  r25 %p\n", r[ 1], r[ 9], r[17], r[25]);
    c1.printf("r02 %p  r10 %p  r18 %p  r26 %p\n", r[ 2], r[10], r[18], r[26]);
    c1.printf("r03 %p  r11 %p  r19 %p  r27 %p\n", r[ 3], r[11], r[19], r[27]);
    c1.printf("r04 %p  r12 %p  r20 %p  r28 %p\n", r[ 4], r[12], r[20], r[28]);
    c1.printf("r05 %p  r13 %p  r21 %p  r29 %p\n", r[ 5], r[13], r[21], r[29]);
    c1.printf("r06 %p  r14 %p  r22 %p  r30 %p\n", r[ 6], r[14], r[22], r[30]);
    c1.printf("r07 %p  r15 %p  r23 %p  r31 %p\n", r[ 7], r[15], r[23], r[31]);
    c1.printf("\n");
  }
  */

  /*
  while(1) {
    for (volatile int i = 0; i < 1000; i++) {}
    hart_pcs[1] = start_hart(2, hart_pcs[2]);
    for (volatile int i = 0; i < 1000; i++) {}
    hart_pcs[2] = start_hart(3, hart_pcs[3]);
    for (volatile int i = 0; i < 1000; i++) {}
    hart_pcs[3] = start_hart(1, hart_pcs[1]);
  }
  */

  /*
  start_hart(1, 0x00400000 - 4);
  for (int i = 0; i < 32; i++) {
    ((volatile uint32_t*)0xE0000000)[32 * 2 + i] = 0xDEADBEEF + i;
  }

  for (int i = 0; i < 32; i++) {
    uint32_t reg = ((volatile uint32_t*)0xE0000000)[32 * 2 + i];
    printf("hart %d reg r%d = 0x%p\n", 2, i, reg);
  }
  */
