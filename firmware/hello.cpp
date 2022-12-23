#include <stdarg.h>

typedef unsigned long uint32_t;
typedef unsigned char uint8_t;

//------------------------------------------------------------------------------

void putchar(char c) {
  *(volatile uint32_t*)0x40000000 = c;
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
    j main
    .option pop
  )");
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  int stack_val = 1234;

  printf("\n");
  printf("main @  0x%p\n", main);
  printf("stack @ 0x%p\n", &stack_val);
  printf("decimal 1234567890 %d\n", 1234567890);
  printf("decimal -123456789 %d\n", -123456789);
  printf("hex     0x12345678 0x%x\n", 0x12345678);
  printf("hex     0x1234     0x%x\n", 0x1234);
  printf("pointer 0x12345678 0x%p\n", 0x12345678);
  printf("pointer 0x00001234 0x%p\n", 0x00001234);
  printf("char    !@#$\\%^&*() %c%c%c%c%c%c%c%c%c%c\n", '!', '@', '#', '$', '%', '^', '&', '*', '(', ')');

  *(volatile uint32_t*)0xFFFFFFF0 = 1;
  __asm__("wfi");
  return 0;
}

//------------------------------------------------------------------------------


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
