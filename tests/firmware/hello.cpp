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

typedef int (*start_func)(void);
extern void* _start;

//------------------------------------------------------------------------------

void print_test(Console* c) {
  c->printf("thread  %d\n",   get_hart());
  c->printf("stack   0x%p\n", get_sp());
  c->printf("global  0x%p\n", get_gp());

  c->printf("decimal 1234567890 %d\n", 1234567890);
  c->printf("decimal -123456789 %d\n", -123456789);
  c->printf("hex     0x12345678 0x%x\n", 0x12345678);
  c->printf("hex     0x1234     0x%x\n", 0x1234);
  c->printf("pointer 0x12345678 0x%p\n", 0x12345678);
  c->printf("pointer 0x00001234 0x%p\n", 0x00001234);
  c->printf("char    !@#$\\%^&*() %c%c%c%c%c%c%c%c%c%c\n", '!', '@', '#', '$', '%', '^', '&', '*', '(', ')');
}

//------------------------------------------------------------------------------

void loop(int hart, Console* c) {
  c->printf("Thread %d started\n", hart);

  int rep = 0;
  while(1) {
    c->printf("\n");
    c->printf("Rep %d\n", rep++);
    print_test(c);
  }
}

//------------------------------------------------------------------------------

void main0() {
  Console* c = &c1;
  c->printf("Thread 0 started\n");

  uint32_t pc1 = 0x80000000 | (1 << 24) | uint32_t(&_start);
  uint32_t pc2 = 0x80000000 | (2 << 24) | uint32_t(&_start);
  uint32_t pc3 = 0x80000000 | (3 << 24) | uint32_t(&_start);

  //c->printf("pc1 0x%x\n", pc1);
  //c->printf("pc2 0x%x\n", pc2);
  //c->printf("pc3 0x%x\n", pc3);

  /*
  for (int i = 0; i < 20000; i++) {
    volatile uint32_t* data_flag = (uint32_t*)0xB0010000;
    volatile uint32_t* data_in   = (uint32_t*)0xB0010004;
    //volatile uint32_t* data_out = (uint32_t*)0x40000000;

    if (*data_flag) {
      c->printf("%c", *data_in);
    }
  }
  c->printf("\n");
  */

  //c->printf("Starting thread-swapping loop\n", get_hart());

  /*
  while(1) {
    pc1 = csr_swap_secondary_thread(pc1);
    c->printf("pc1 0x%x\n", pc1);
  }
  */

  csr_swap_secondary_thread(pc1);
  while(1) {
    pc1 = csr_swap_secondary_thread(pc2);
    pc2 = csr_swap_secondary_thread(pc3);
    pc3 = csr_swap_secondary_thread(pc1);
  }
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  switch(get_hart()) {
    case 0: main0(); break;
    case 1: loop(1, &c2); break;
    case 2: loop(2, &c3); break;
    case 3: loop(3, &c4); break;
  }
  return 0;
}

//------------------------------------------------------------------------------
