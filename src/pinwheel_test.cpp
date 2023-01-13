#include "CoreLib/Tests.h"
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

TestResults run_test(const char* test_name, int reps, int max_cycles) {
  TEST_INIT("'%6s', %d reps: ", test_name, reps);

  double time = 0;
  int tocks = 0;
  double time_a, time_b;
  int elapsed_cycles = 0;

  pinwheel top;

  char firmware_filename[256];
  sprintf(firmware_filename, "rv_tests/%s.elf", test_name);

  top.load_elf(firmware_filename);

  for (int rep = 0; rep < reps; rep++) {
    top.tock_twocycle(1);
    top.tick_twocycle(1);
    time -= timestamp();
    for (elapsed_cycles = 0; elapsed_cycles < max_cycles; elapsed_cycles++) {
      top.tock_twocycle(0);
      top.tick_twocycle(0);
      tocks++;

      if (top.get_debug()) {
        EXPECT_EQ(top.get_debug(), 1, "FAIL @ %d", elapsed_cycles);
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

TestResults test_instructions(int reps, int max_cycles) {
  TEST_INIT("Testing %d reps up to %d cycles", reps, max_cycles);

  const char* instructions[38] = {
    "add", "addi", "and", "andi", "auipc", "beq",  "bge", "bgeu",
    "blt", "bltu", "bne", "jal",  "jalr",  "lb",   "lbu", "lh",
    "lhu", "lui",  "lw",  "or",   "ori",   "sb",   "sh",  "simple",
    "sll", "slli", "slt", "slti", "sltiu", "sltu", "sra", "srai",
    "srl", "srli", "sub", "sw",   "xor",   "xori"
  };

  const int instruction_count = sizeof(instructions) / sizeof(instructions[0]);

  for (int i = 0; i < instruction_count; i++) {
    clear_cache();
    results += run_test(instructions[i], reps, max_cycles);
  }
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
  results += test_instructions(reps, max_cycles);
  results.dump();

  LOG_B("Total tocks %f\n", double(total_tocks));
  LOG_B("Total time %f\n", double(total_time));
  double rate = double(total_tocks) / double(total_time);
  LOG_G("Sim rate %f mhz\n", rate / 1.0e6);

  return results.test_fail ? 0 : 1;
}

//------------------------------------------------------------------------------
