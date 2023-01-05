#include "CoreLib/Tests.h"
#include "pinwheel.h"
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

bool load_elf(const char* filename) {
  struct stat sb;
  if (stat(filename, &sb) == -1) {
    return false;
  }

  uint8_t* blob = new uint8_t[sb.st_size];
  FILE* f = fopen(filename, "rb");
  auto result = fread(blob, 1, sb.st_size, f);
  fclose(f);

  char text_name[300];
  char data_name[300];
  sprintf(text_name, "%s.text", filename);
  sprintf(data_name, "%s.data", filename);

  char text_arg[400];
  char data_arg[400];
  sprintf(text_arg, "+text_file=%s", text_name);
  sprintf(data_arg, "+data_file=%s", data_name);

  const char* argv2[2] = {
    text_arg,
    data_arg
  };
  metron_init(2, argv2);

  Elf32_Ehdr& header = *(Elf32_Ehdr*)blob;

  for (int i = 0; i < header.e_phnum; i++) {
    Elf32_Phdr& phdr = *(Elf32_Phdr*)(blob + header.e_phoff + header.e_phentsize * i);
    if (phdr.p_type & PT_LOAD) {
      if (phdr.p_flags & PF_X) {
        //LOG_G("Code @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);
        put_cache(text_name, blob + phdr.p_offset, phdr.p_filesz);
      }
      else if (phdr.p_flags & PF_W) {
        //LOG_G("Data @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);
        put_cache(data_name, blob + phdr.p_offset, phdr.p_filesz);
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------

TestResults run_test(const char* test_name, int reps, int max_cycles) {
  TEST_INIT("'%6s', %d reps: ", test_name, reps);

  double time = 0;
  int tocks = 0;
  double time_a, time_b;
  int elapsed_cycles = 0;

  Pinwheel top;

  for (int rep = 0; rep < reps; rep++) {
    //top.tick_onecycle(1);
    top.tick_twocycle(1);
    time -= timestamp();
    for (elapsed_cycles = 0; elapsed_cycles < max_cycles; elapsed_cycles++) {
      //top.tick_onecycle(0);
      top.tick_twocycle(0);
      tocks++;

      if (top.debug_reg) {
        EXPECT_EQ(top.debug_reg, 1, "FAIL @ %d", elapsed_cycles);
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

    char elf_name[256];
    sprintf(elf_name, "rv_tests/%s.elf", instructions[i]);
    load_elf(elf_name);

    results += run_test(instructions[i], reps, max_cycles);
  }
  TEST_DONE("All instructions pass");
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
