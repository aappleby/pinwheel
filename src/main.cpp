#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>

//#include "Platform.h"
//#include "Tests.h"
//#include "submodules/CLI11/include/CLI/App.hpp"
//#include "submodules/CLI11/include/CLI/Config.hpp"
//#include "submodules/CLI11/include/CLI/Formatter.hpp"
#include "pinwheel.h"

double timestamp() {
  static uint64_t origin = 0;
  timespec ts;
  (void)timespec_get(&ts, TIME_UTC);
  uint64_t now = ts.tv_sec * 1000000000ull + ts.tv_nsec;
  //if (origin == 0) origin = now;
  return double(now - origin) / 1.0e9;
}

//------------------------------------------------------------------------------

const char* instructions[38] = {
  "add", "addi", "and", "andi", "auipc", "beq",  "bge", "bgeu",
  "blt", "bltu", "bne", "jal",  "jalr",  "lb",   "lbu", "lh",
  "lhu", "lui",  "lw",  "or",   "ori",   "sb",   "sh",  "simple",
  "sll", "slli", "slt", "slti", "sltiu", "sltu", "sra", "srai",
  "srl", "srli", "sub", "sw",   "xor",   "xori"
};

/*
const char* instructions[1] = {
  "simple"
};
*/

const int instruction_count = sizeof(instructions) / sizeof(instructions[0]);


//------------------------------------------------------------------------------

uint64_t total_tocks = 0;
double total_time = 0;

void test_instruction(const char* test_name, const int reps,
                             int max_cycles) {
  //TEST_INIT("Testing op %6s, %d reps: ", test_name, reps);
  printf("Testing op %6s, %d reps: ", test_name, reps);

  char buf1[256];
  char buf2[256];
  sprintf(buf1, "+text_file=rv_tests/%s.text.vh", test_name);
  sprintf(buf2, "+data_file=rv_tests/%s.data.vh", test_name);
  const char* argv2[2] = {buf1, buf2};

  metron_init(2, argv2);

  int elapsed_cycles = 0;

  Pinwheel top;

  double time_a, time_b;

  for (int rep = 0; rep < reps; rep++) {
    time_a = timestamp();
    top.tock(1);
    for (elapsed_cycles = 0; elapsed_cycles < max_cycles; elapsed_cycles++) {
      top.tock(0);
      total_tocks++;

      if (top.sig_p12.dbus_addr == 0xfffffff0 && top.sig_p12.dbus_wmask) {
        if (top.sig_p12.dbus_wdata == 0) {
          //TEST_FAIL("FAIL @ %d\n", elapsed_cycles);
          printf("FAIL @ %d\n", elapsed_cycles);
        }
        if (rep == 0) {
          //printf("pass on hart %d at cycle %d\n", top.sig_p12.hart, elapsed_cycles);
        }
        break;
      }
    }
    time_b = timestamp();
    //printf("%f\n", (time_b - time_a));
    total_time += time_b - time_a;
    if (elapsed_cycles == max_cycles) {
      //TEST_FAIL("TIMEOUT\n");
      printf("TIMEOUT\n");
      return;
    }
  }

  //TEST_PASS();
  printf("TEST_PASS\n");
}

//------------------------------------------------------------------------------

int main(int argc, const char** argv) {
  //CLI::App app{"Simple test and benchmark for rvsimple"};

  //int reps = 1000;
  int reps = 10;
  int max_cycles = 10000;

  //app.add_option("-r,--reps", reps, "How many times to repeat the test");
  //app.add_option("-m,--max_cycles", max_cycles,
  //               "Maximum # cycles to simulate before timeout");
  //CLI11_PARSE(app, argc, argv);

  //LOG_B("Starting %s @ %d reps...\n", argv[0], reps);
  printf("Starting %s @ %d reps...\n", argv[0], reps);

  total_tocks = 0;
  total_time = 0;

  printf("Testing...\n");
  //TestResults results("main");
  for (int i = 0; i < instruction_count; i++) {
    //results << test_instruction(instructions[i], reps, max_cycles);
    test_instruction(instructions[i], reps, max_cycles);
  }

  printf("Total tocks %f\n", double(total_tocks));
  printf("Total time %f\n", double(total_time));

  double rate = double(total_tocks) / double(total_time);
  printf("Sim rate %f mhz\n", rate / 1.0e6);

  //return results.show_banner();
  return 0;
}
