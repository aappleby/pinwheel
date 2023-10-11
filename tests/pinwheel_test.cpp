#include "metrolib/src/CoreLib/Tests.h"
#include "pinwheel.h"

//------------------------------------------------------------------------------

double timestamp() {
  static uint64_t origin = 0;
  timespec ts;
  (void)timespec_get(&ts, TIME_UTC);
  uint64_t now = ts.tv_sec * 1000000000ull + ts.tv_nsec;
  if (origin == 0) origin = now;
  return double(now - origin) / 1.0e9;
}

uint64_t total_tocks = 0;
double total_time = 0;

//------------------------------------------------------------------------------

TestResults run_test_elf(const char* test_filename, int reps = 1, int max_cycles = 100000, bool expect_fail = false) {
  TEST_INIT("'%-30s', %d reps: ", test_filename, reps);

  double time = 0;
  int tocks = 0;
  double time_a, time_b;
  int elapsed_cycles = 0;

  pinwheel top;

  top.load_elf(test_filename);

  for (int rep = 0; rep < reps; rep++) {
    top.tock(1, 0, 0);
    top.tick(1, 0, 0);
    time -= timestamp();
    for (elapsed_cycles = 0; elapsed_cycles < max_cycles; elapsed_cycles++) {
      top.tock(0, 0, 0);
      top.tick(0, 0, 0);
      tocks++;

      if (top.get_debug()) {
        if (expect_fail) {
          EXPECT_NE(top.get_debug(), 1, "FAIL @ %d", elapsed_cycles);
        }
        else {
          EXPECT_EQ(top.get_debug(), 1, "FAIL @ %d", elapsed_cycles);
        }
        break;
      }
    }
    time += timestamp();
    ASSERT_NE(elapsed_cycles, max_cycles, "TIMEOUT");
  }

  total_time += time;
  total_tocks += tocks;
  TEST_DONE("pass @ %7d tocks, %6.2f msec", tocks, time * 1000.0f);
}

//------------------------------------------------------------------------------

TestResults run_rv32i_tests(int reps, int max_cycles) {
  TEST_INIT("Testing all rv32i instructions");

  const char* instructions[38] = {
    "add", "addi", "and", "andi", "auipc", "beq",  "bge", "bgeu",
    "blt", "bltu", "bne", "jal",  "jalr",  "lb",   "lbu", "lh",
    "lhu", "lui",  "lw",  "or",   "ori",   "sb",   "sh",  "simple",
    "sll", "slli", "slt", "slti", "sltiu", "sltu", "sra", "srai",
    "srl", "srli", "sub", "sw",   "xor",   "xori"
  };

  const int instruction_count = sizeof(instructions) / sizeof(instructions[0]);

  for (int i = 0; i < instruction_count; i++) {
    char firmware_filename[256];
    sprintf(firmware_filename, "rv_tests/%s.elf", instructions[i]);
    results += run_test_elf(firmware_filename, reps, max_cycles);
  }
  TEST_DONE();
}

//------------------------------------------------------------------------------

TestResults run_microtests() {
  TEST_INIT("Running microtests");
  results += run_test_elf("bin/tests/basic");
  results += run_test_elf("bin/tests/call_jalr");
  results += run_test_elf("bin/tests/get_hart");
  results += run_test_elf("bin/tests/start_thread");
  results += run_test_elf("bin/tests/stepping");
  results += run_test_elf("bin/tests/write_regs");
  results += run_test_elf("bin/tests/yield");
  results += run_test_elf("bin/tests/read_regs");
  results += run_test_elf("bin/tests/write_code");
  TEST_DONE();
}

//------------------------------------------------------------------------------

int main(int argc, const char** argv) {

#ifdef CONFIG_RELEASE
  int reps = 1000;
#else
  int reps = 1;
#endif

  int max_cycles = 100000;

  LOG_B("Starting %s @ %d reps...\n", argv[0], reps);

  total_tocks = 0;
  total_time = 0;

  TestResults results;
  results += run_rv32i_tests(reps, max_cycles);
  results += run_microtests();
  results.dump();

  LOG_B("Total tocks %f\n", double(total_tocks));
  LOG_B("Total time %f\n", double(total_time));
  double rate = double(total_tocks) / double(total_time);
  LOG_G("Sim rate %f mhz\n", rate / 1.0e6);

  return results.test_fail ? 1 : 0;
}

//------------------------------------------------------------------------------
