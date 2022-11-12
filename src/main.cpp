#include "Tests.h"

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>

//#include "Platform.h"
#include "Tests.h"
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

TestResults test_blah2() {
  static int blep = 0;
  TestResults results;
  blep++;
  EXPECT_NE(1, 4, "slkdjf");
  return results;
  //TestResults results;
  //EXPECT_EQ(1, 2, "What?")
  //return results;
}

TestResults test_blah() {
  TestResults results;
  results += test_blah2();
  ASSERT_NE(0, 1, "why?");
  return results;
  //TestResults results;
  //EXPECT_EQ(1, 2, "What?")
  //return results;
}

TestResults test_instruction(const char* test_name, const int reps,
                             int max_cycles) {
  TEST_INIT("Testing op %6s, %d reps: ", test_name, reps);

  results += test_blah();

  if (strcmp(test_name, "sub") == 0) {
    ASSERT_EQ(1, 0, "slkdjf");
  }

  /*
  if (strcmp(test_name, "jalr") == 0) {
    ASSERT_EQ(1, 0, "slkdjf");
  }
  */

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
        EXPECT_EQ(top.sig_p12.dbus_wdata, 1, "FAIL @ %d", elapsed_cycles);
        /*
        if (top.sig_p12.dbus_wdata == 0) {
          TEST_FAIL("FAIL @ %d", elapsed_cycles);
        }
        if (rep == 0) {
          //printf("pass on hart %d at cycle %d\n", top.sig_p12.hart, elapsed_cycles);
        }
        */
        break;
      }
    }
    time_b = timestamp();
    //printf("%f\n", (time_b - time_a));
    total_time += time_b - time_a;
    ASSERT_NE(elapsed_cycles, max_cycles, "TIMEOUT");
  }

  //TEST_DONE();
  TEST_DONE("op %6s pass", test_name);
}

//------------------------------------------------------------------------------

TestResults test_instructions(int reps, int max_cycles) {
  TEST_INIT("Testing %d reps up to %d cycles", reps, max_cycles);
  for (int i = 0; i < instruction_count; i++) {
    results += test_instruction(instructions[i], reps, max_cycles);
  }
  TEST_DONE();
  //TEST_DONE("All instructions pass");
}

int main(int argc, const char** argv) {
  int reps = 2;
  int max_cycles = 10000;

  LOG_B("Starting %s @ %d reps...\n", argv[0], reps);

  total_tocks = 0;
  total_time = 0;

  TestResults results;
  results += test_instructions(reps, max_cycles);
  results.dump();
  //LOG_G("  %d passing checks, %d failing checks\n", results.check_pass, results.check_fail);
  //LOG_G("  %d passing tests, %d failing tests\n", results.test_pass, results.test_fail);

  LOG_B("Total tocks %f\n", double(total_tocks));
  LOG_B("Total time %f\n", double(total_time));

  double rate = double(total_tocks) / double(total_time);
  LOG_G("Sim rate %f mhz\n", rate / 1.0e6);

  return results.test_fail ? 0 : 1;
}
