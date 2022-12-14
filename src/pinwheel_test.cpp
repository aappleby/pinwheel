#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <elf.h>
#include <sys/stat.h>

#include "Tests.h"
#include "pinwheel.h"
//#include "pinwheel_demo.h"

//#include "AppLib/AppHost.h"

double timestamp() {
  static uint64_t origin = 0;
  timespec ts;
  (void)timespec_get(&ts, TIME_UTC);
  uint64_t now = ts.tv_sec * 1000000000ull + ts.tv_nsec;
  if (origin == 0) origin = now;
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

const int instruction_count = sizeof(instructions) / sizeof(instructions[0]);

//------------------------------------------------------------------------------

uint64_t total_tocks = 0;
double total_time = 0;

TestResults test_instruction(const char* test_name, const int reps,
                             int max_cycles) {
  TEST_INIT("Testing op %6s, %d reps: ", test_name, reps);

  char buf1[256];
  char buf2[256];
  sprintf(buf1, "+text_file=rv_tests/%s.text.vh", test_name);
  sprintf(buf2, "+data_file=rv_tests/%s.data.vh", test_name);
  const char* argv2[2] = {buf1, buf2};

  metron_init(2, argv2);

  int elapsed_cycles = 0;

  Pinwheel top;

  double time = 0;
  int tocks = 0;
  double time_a, time_b;

  for (int rep = 0; rep < reps; rep++) {
    top.tock(1);
    time -= timestamp();
    for (elapsed_cycles = 0; elapsed_cycles < max_cycles; elapsed_cycles++) {
      top.tock(0);
      tocks++;

      if (top.data_mem[0x3FC]) {
        EXPECT_EQ(top.data_mem[0x3FC], 1, "FAIL @ %d", elapsed_cycles);
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
  for (int i = 0; i < instruction_count; i++) {
    results += test_instruction(instructions[i], reps, max_cycles);
  }
  TEST_DONE("All instructions pass");
}

//------------------------------------------------------------------------------

TestResults test_elf(uint8_t* blob, int size, int reps, int max_cycles) {
  TEST_INIT();
  Pinwheel top;

  const char* argv2[2] = {
    "+text_file=rv_tests/test_elf.text.vh",
    "+data_file=rv_tests/test_elf.data.vh"
  };
  metron_init(2, argv2);

  Elf32_Ehdr& header = *(Elf32_Ehdr*)blob;

  for (int i = 0; i < header.e_phnum; i++) {
    Elf32_Phdr& phdr = *(Elf32_Phdr*)(blob + header.e_phoff + header.e_phentsize * i);
    if (phdr.p_type & PT_LOAD) {
      if (phdr.p_vaddr == 0x00000000) {
        put_cache("rv_tests/test_elf.text.vh", blob + phdr.p_align, phdr.p_filesz);
      }
      if (phdr.p_vaddr == 0x80000000) {
        put_cache("rv_tests/test_elf.data.vh", blob + phdr.p_offset, phdr.p_filesz);
      }
    }
  }

  double time = 0;
  int tocks = 0;
  double time_a, time_b;
  int elapsed_cycles = 0;

  for (int rep = 0; rep < reps; rep++) {
    top.tock(1);
    time -= timestamp();
    for (elapsed_cycles = 0; elapsed_cycles < max_cycles; elapsed_cycles++) {
      top.tock(0);
      tocks++;

      if (top.data_mem[0x3FC]) {
        EXPECT_EQ(top.data_mem[0x3FC], 1, "FAIL @ %d", elapsed_cycles);
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
  const char* firmware_filename = "firmware/bin/hello";
  struct stat sb;
  if (stat(firmware_filename, &sb) == -1) {
    printf("Could not stat firmware %s\n", firmware_filename);
    return 0;
  }
  uint8_t* blob = new uint8_t[sb.st_size];
  FILE* f = fopen(firmware_filename, "rb");
  auto result = fread(blob, 1, sb.st_size, f);
  fclose(f);

  //test_elf(blob, sb.st_size, 1, 10);

  int reps = 1000;
  //int reps = 1;
  int max_cycles = 1425;

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

  delete [] blob;

  return results.test_fail ? 0 : 1;
}

//------------------------------------------------------------------------------
