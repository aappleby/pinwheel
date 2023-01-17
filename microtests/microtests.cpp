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

TestResults run_microtest(const char* test_filename, int reps = 1, int max_cycles = 100000, bool expect_fail = false) {
  TEST_INIT("'%-20s', %d reps: ", test_filename, reps);

  double time = 0;
  int tocks = 0;
  double time_a, time_b;
  int elapsed_cycles = 0;

  pinwheel top;

  top.load_elf(test_filename);

  for (int rep = 0; rep < reps; rep++) {
    top.tock(1);
    top.tick(1);
    time -= timestamp();
    for (elapsed_cycles = 0; elapsed_cycles < max_cycles; elapsed_cycles++) {
      top.tock(0);
      top.tick(0);
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

int main(int argc, const char** argv) {

  LOG_B("Starting %s...\n", argv[0]);

  total_tocks = 0;
  total_time = 0;

  TestResults results;
  results += run_microtest("bin/basic");
  results += run_microtest("bin/call_jalr");
  results += run_microtest("bin/get_hart");
  results += run_microtest("bin/start_thread");
  results += run_microtest("bin/stepping");
  results += run_microtest("bin/write_regs");
  results += run_microtest("bin/yield");
  results.dump();

  LOG_B("Total tocks %f\n", double(total_tocks));
  LOG_B("Total time %f\n", double(total_time));
  double rate = double(total_tocks) / double(total_time);
  LOG_G("Sim rate %f mhz\n", rate / 1.0e6);

  return results.test_fail ? 1 : 0;
}

//------------------------------------------------------------------------------
