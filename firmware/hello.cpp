#include <stdarg.h>

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
uint32_t start_hart1(uint32_t address) {
  __asm__(R"(
    .insn r 0x0B, 0, 0, a0, a0, x0
    ret
  )");
}

//------------------------------------------------------------------------------

volatile uint32_t& out_c = *(volatile uint32_t*)0x40000000;

void puts(const char* s) {
  while(*s) {
    out_c = *s;
    s++;
  }
}

void putnx(uint8_t x) {
  out_c = (x > 9 ? 'A' - 10 : '0') + x;
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
    out_c = '0';
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
    out_c = '0';
    return;
  }

  if (d < 0) {
    out_c = '-';
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
    out_c = '0' + buf[i];
  }
}

void printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  while(*format) {
    if (*format == '\\') {
      format++;
      out_c = *format++;
    }
    else if (*format == '%') {
      format++;
      switch(*format++) {
      case 'c': out_c = (char)va_arg(args, int); break;
      case 'd': putd(va_arg(args, int)); break;
      case 'x': putx(va_arg(args, uint32_t)); break;
      case 'p': putdx(va_arg(args, uint32_t)); break;
      case 's': puts(va_arg(args, const char*)); break;
      }
    } else {
      out_c = *format++;
    }
  }
}

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
    sll t0, t0, 9
    sub sp, sp, t0
  main_loop:
    call main
    j main_loop
  .option pop
  )");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  //int hart = get_hart();

  for (int i = 0; i < 100; i++) {
    ((volatile uint32_t*)0xE0000000)[37] = 0xF00DCAFE + i;
  }

  //printf("\n");

#if 0
  if (hart != 0) {
    for (int i = 0; true; i++) {
      printf("<%d>", i);
    }
  }


  printf("hart    %d\n", hart);
  printf("main    0x%p\n", main);
  printf("stack   0x%p\n", get_sp());
  printf("global  0x%p\n", get_gp());

  printf("decimal 1234567890 %d\n", 1234567890);
  printf("decimal -123456789 %d\n", -123456789);
  printf("hex     0x12345678 0x%x\n", 0x12345678);
  printf("hex     0x1234     0x%x\n", 0x1234);
  printf("pointer 0x12345678 0x%p\n", 0x12345678);
  printf("pointer 0x00001234 0x%p\n", 0x00001234);
  printf("char    !@#$\\%^&*() %c%c%c%c%c%c%c%c%c%c\n", '!', '@', '#', '$', '%', '^', '&', '*', '(', ')');

  printf("running hart 1 for a bit\n");
  static uint32_t hart1_pc = 0x00400000 - 4;
  start_hart1(hart1_pc);
  for (volatile int i = 0; i < 100; i++) {}
  hart1_pc = start_hart1(0);
  printf("\n");
  printf("running hart 1 done\n");
  printf("hart1_pc = 0x%p\n", hart1_pc);
#endif

  /*
  for (int hart = 1; hart < 4; hart++) {
    //uint32_t* r = (uint32_t*)(0x10000000 + (hart*128));
    printf("hart %d\n", hart);
    printf("r00 %p  r08 %p  r16 %p  r24 %p\n", r[ 0], r[ 8], r[16], r[24]);
    printf("r01 %p  r09 %p  r17 %p  r25 %p\n", r[ 1], r[ 9], r[17], r[25]);
    printf("r02 %p  r10 %p  r18 %p  r26 %p\n", r[ 2], r[10], r[18], r[26]);
    printf("r03 %p  r11 %p  r19 %p  r27 %p\n", r[ 3], r[11], r[19], r[27]);
    printf("r04 %p  r12 %p  r20 %p  r28 %p\n", r[ 4], r[12], r[20], r[28]);
    printf("r05 %p  r13 %p  r21 %p  r29 %p\n", r[ 5], r[13], r[21], r[29]);
    printf("r06 %p  r14 %p  r22 %p  r30 %p\n", r[ 6], r[14], r[22], r[30]);
    printf("r07 %p  r15 %p  r23 %p  r31 %p\n", r[ 7], r[15], r[23], r[31]);
  }
  */



  *(volatile uint32_t*)0xFFFFFFF0 = 1;
  return 0;
}

//------------------------------------------------------------------------------
