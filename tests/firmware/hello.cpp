#include <stdarg.h>
#include "utils.h"

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

Console c1 = { (uint32_t*)0x40000000 };
Console c2 = { (uint32_t*)0x50000000 };
Console c3 = { (uint32_t*)0x60000000 };
Console c4 = { (uint32_t*)0x70000000 };

//------------------------------------------------------------------------------

typedef int (*start_func)(void);
extern void* _start;

int main(int argc, char** argv) {
  uint32_t start_pc = uint32_t(&_start);
  int hart = get_hart();

  Console* c = nullptr;
  if (hart == 0) c = &c1;
  if (hart == 1) c = &c2;
  if (hart == 2) c = &c3;
  if (hart == 3) c = &c4;

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

  //if (hart < 3) {
    uint32_t new_start = ((((hart + 1) % 4) << 24) | start_pc);
    csr_swap_secondary_thread(new_start);
    c->printf("Hart %d started at %p\n", hart + 1, new_start);

    int x = 0;
    while(1) {
      c->printf("%d ", x++);
    }
  /*
    //reinterpret_cast<start_func>(new_start)();
  }
  else {
    *(volatile uint32_t*)0xFFFFFFF0 = 1;
    c->printf("Test pass\n\n\n");
    return 0;
  }
  */
  return 0;
}

//------------------------------------------------------------------------------
