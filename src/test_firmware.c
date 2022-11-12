#include <stdint.h>

#define THREAD_SCHEDULE *(volatile uint32_t*)0x30000000
#define GPIO_OUT0 *(volatile uint32_t*)0x10000000
#define GPIO_OUT1 *(volatile uint32_t*)0x10000004
#define GPIO_OUT2 *(volatile uint32_t*)0x10000008
#define GPIO_OUT3 *(volatile uint32_t*)0x1000000C
#define GPIO_OUT4 *(volatile uint32_t*)0x10000010
#define GPIO_OUT5 *(volatile uint32_t*)0x10000014
#define GPIO_OUT6 *(volatile uint32_t*)0x10000018
#define GPIO_OUT7 *(volatile uint32_t*)0x1000001C

__attribute__ ((noinline)) void delay(uint32_t count) {
delay_loop:
  asm("add %0, %0, -1" : "=r"(count) : "r"(count) : );
  asm goto("bnez %0, delay" : : "r"(count) : : delay_loop);
}

/////////////////////////////////////////////////////////
// Each LED is being blinked by a separate virtual core
/////////////////////////////////////////////////////////

void main0(int argc, char** argv) {
  // set thread schedule to round-robin all 4 cores
  THREAD_SCHEDULE = 0xE4E4E4E4;

  while(1) {
    GPIO_OUT0 = 1;
    delay(1000);
    GPIO_OUT0 = 0;
    delay(100000);
  }
}

void main1(int argc, char** arv) {
  while(1) {
    GPIO_OUT2 = 1;
    delay(1700);
    GPIO_OUT2 = 0;
    delay(100000);
  }
}

void main2(int argc, char** arv) {
  while(1) {
    GPIO_OUT5 = 1;
    delay(2400);
    GPIO_OUT5 = 0;
    delay(100000);
  }
}

void main3(int argc, char** arv) {
  while(1) {
    GPIO_OUT7 = 1;
    delay(3100);
    GPIO_OUT7 = 0;
    delay(100000);
  }
}