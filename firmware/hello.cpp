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

//------------------------------------------------------------------------------

static int put_count = 0;

void putchar(char c) {
  *(volatile uint32_t*)0x40000000 = c;
  put_count++;
}

void puts(const char* s) {
  while(*s) {
    putchar(*s);
    s++;
  }
}

void putnx(uint8_t x) {
  putchar((x > 9 ? 'A' - 10 : '0') + x);
}

void putbx(uint8_t x) {
  putnx((x >> 4) & 0x0F);
  putnx((x >> 0) & 0x0F);
}

void putdx(uint32_t x) {
  putbx((x >> 24) & 0xFF);
  putbx((x >> 16) & 0xFF);
  putbx((x >>  8) & 0xFF);
  putbx((x >>  0) & 0xFF);
}

void putx(uint32_t x) {
  if (x >= 16) putx(x >> 4);
  putnx(x & 0xF);
}

void putd(int d) {
  if (d < 0) {
    putchar('-');
    putd(-d);
  }
  else {
    if (d > 10) putd(d / 10);
    putchar('0' + (d % 10));
  }
}

void printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  while(*format) {
    if (*format == '\\') {
      format++;
      putchar(*format++);
    }
    else if (*format == '%') {
      format++;
      switch(*format++) {
      case 'c': putchar((char)va_arg(args, int)); break;
      case 'd': putd(va_arg(args, int)); break;
      case 'x': putx(va_arg(args, uint32_t)); break;
      case 'p': putdx(va_arg(args, uint32_t)); break;
      case 's': puts(va_arg(args, const char*)); break;
      }
    } else {
      putchar(*format++);
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
    main_loop:
    call main
    j main_loop
    .option pop
  )");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
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

  //for (int i = 32; i < 64; i++) {
  //  *(uint32_t*)(0x10000000 + (i * 4)) = 0xF00DCAFE;
  //}
  *(uint32_t*)(0x10000080) = 0xF00DCAFE;
  *(uint32_t*)(0x10000084) = 0xF00DCAFE;
  *(uint32_t*)(0x10000088) = 0xF00DCAFE;
  *(uint32_t*)(0x1000008C) = 0xF00DCAFE;
  *(uint32_t*)(0x10000090) = 0xF00DCAFE;
  *(uint32_t*)(0x10000094) = 0xF00DCAFE;
  *(uint32_t*)(0x10000098) = 0xF00DCAFE;
  *(uint32_t*)(0x1000009C) = 0xF00DCAFE;
  *(uint32_t*)(0x100000A0) = 0xF00DCAFE;
  *(uint32_t*)(0x100000A4) = 0xF00DCAFE;
  *(uint32_t*)(0x100000A8) = 0xF00DCAFE;
  *(uint32_t*)(0x100000AC) = 0xF00DCAFE;
  *(uint32_t*)(0x100000B0) = 0xF00DCAFE;
  *(uint32_t*)(0x100000B4) = 0xF00DCAFE;
  *(uint32_t*)(0x100000B8) = 0xF00DCAFE;
  *(uint32_t*)(0x100000BC) = 0xF00DCAFE;
  *(uint32_t*)(0x100000C0) = 0xF00DCAFE;
  *(uint32_t*)(0x100000C4) = 0xF00DCAFE;
  *(uint32_t*)(0x100000C8) = 0xF00DCAFE;
  *(uint32_t*)(0x100000CC) = 0xF00DCAFE;
  *(uint32_t*)(0x100000D0) = 0xF00DCAFE;
  *(uint32_t*)(0x100000D4) = 0xF00DCAFE;
  *(uint32_t*)(0x100000D8) = 0xF00DCAFE;
  *(uint32_t*)(0x100000DC) = 0xF00DCAFE;
  *(uint32_t*)(0x100000E0) = 0xF00DCAFE;
  *(uint32_t*)(0x100000E4) = 0xF00DCAFE;
  *(uint32_t*)(0x100000E8) = 0xF00DCAFE;
  *(uint32_t*)(0x100000EC) = 0xF00DCAFE;
  *(uint32_t*)(0x100000F0) = 0xF00DCAFE;
  *(uint32_t*)(0x100000F4) = 0xF00DCAFE;
  *(uint32_t*)(0x100000F8) = 0xF00DCAFE;
  *(uint32_t*)(0x100000FC) = 0xF00DCAFE;

  /*
  uint32_t* hart2_regs = (uint32_t*)(0x10000000 + (2*128));
  for (int i = 0; i < 32; i++) {
    hart2_regs[i] = 0x01010101 * i;
  }
  */

  for (int hart = 1; hart < 4; hart++) {
    //uint32_t* r = (uint32_t*)(0x10000000 + (hart*128));
    printf("hart %d\n", hart);
    /*
    printf("r00 %p  r08 %p  r16 %p  r24 %p\n", r[ 0], r[ 8], r[16], r[24]);
    printf("r01 %p  r09 %p  r17 %p  r25 %p\n", r[ 1], r[ 9], r[17], r[25]);
    printf("r02 %p  r10 %p  r18 %p  r26 %p\n", r[ 2], r[10], r[18], r[26]);
    printf("r03 %p  r11 %p  r19 %p  r27 %p\n", r[ 3], r[11], r[19], r[27]);
    printf("r04 %p  r12 %p  r20 %p  r28 %p\n", r[ 4], r[12], r[20], r[28]);
    printf("r05 %p  r13 %p  r21 %p  r29 %p\n", r[ 5], r[13], r[21], r[29]);
    printf("r06 %p  r14 %p  r22 %p  r30 %p\n", r[ 6], r[14], r[22], r[30]);
    printf("r07 %p  r15 %p  r23 %p  r31 %p\n", r[ 7], r[15], r[23], r[31]);
    */
  }

  /*
  for (int i = 0; i < 64; i++) {
    printf("hart %d reg %d 0x%p\n", (i >> 5), (i & 31), *(uint32_t*)(0x10000000 + (i*4)));
    if ((i & 7) == 7) printf("\n");
  }

  for (int i = 0; i < 64; i++) {
    printf("hart %d reg %d 0x%p\n", (i >> 5), (i & 31), *(uint32_t*)(0x10000000 + (i*4)));
    if ((i & 7) == 7) printf("\n");
  }
  */

  printf("printed %d\n", put_count);
  printf("\n");

  *(volatile uint32_t*)0xFFFFFFF0 = 1;
  return 0;
}

//------------------------------------------------------------------------------
