#include "metrolib/core/Tests.h"
#include "pinwheel/soc/pinwheel_soc.h"

#include <elf.h>
#include <sys/stat.h>

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

TestResults run_test_hex(const char* code_filename, const char* data_filename, int reps = 1, int max_cycles = 100000, bool expect_fail = false) {
  TEST_INIT("'%-30s', %d reps: ", code_filename, reps);

  double time = 0;
  int tocks = 0;
  double time_a, time_b;
  int elapsed_cycles = 0;

  pinwheel_soc top(code_filename, data_filename, "pinwheel/uart/message.hex");

  for (int rep = 0; rep < reps; rep++) {
    top.tock(1);
    double time_a = timestamp();
    for (elapsed_cycles = 0; elapsed_cycles < max_cycles; elapsed_cycles++) {
      top.tock(0);
      tocks++;

      if (top.core.data_tla.a_address == 0xFFFFFFF0 && top.core.data_tla.a_opcode == TL::PutFullData) {
        if (expect_fail) {
          EXPECT_NE(top.get_debug(), 1, "FAIL @ %d", elapsed_cycles);
        }
        else {
          EXPECT_EQ(top.get_debug(), 1, "FAIL @ %d", elapsed_cycles);
        }
        break;
      }
    }
    double time_b = timestamp();
    total_time += time_b - time_a;
    ASSERT_NE(elapsed_cycles, max_cycles, "TIMEOUT");
  }

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
    char code_filename[256];
    char data_filename[256];
    sprintf(code_filename, "gen/tests/rv_tests/%s.code.vh", instructions[i]);
    sprintf(data_filename, "gen/tests/rv_tests/%s.data.vh", instructions[i]);
    results << run_test_hex(code_filename, data_filename, reps, max_cycles);
  }
  TEST_DONE();
}

//------------------------------------------------------------------------------

TestResults run_microtests(int reps, int max_cycles) {
  TEST_INIT("Running microtests");

  const char* tests[] = {
    "basic", "call_jalr", "get_hart", "start_thread",
    "stepping", "write_regs", "yield", "read_regs",
    "write_code"
  };

  const int test_count = sizeof(tests) / sizeof(tests[0]);

  for (int i =0; i < test_count; i++) {
    char code_filename[256];
    char data_filename[256];
    sprintf(code_filename, "gen/tests/firmware/%s.code.vh", tests[i]);
    sprintf(data_filename, "gen/tests/firmware/%s.data.vh", tests[i]);
    results << run_test_hex(code_filename, data_filename, reps, max_cycles);
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
  results << run_rv32i_tests(reps, max_cycles);
  results << run_microtests(reps, max_cycles);
  results.show_result();

  LOG_B("Total tocks %f\n", double(total_tocks));
  LOG_B("Total time %f\n", double(total_time));
  double rate = double(total_tocks) / double(total_time);
  LOG_G("Sim rate %f mhz\n", rate / 1.0e6);

  return results.test_fail ? 1 : 0;
}

//------------------------------------------------------------------------------
