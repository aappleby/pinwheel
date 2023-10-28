#include "metrolib/core/Tests.h"
#include "pinwheel/metron/pinwheel.h"

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

bool load_elf(const char* firmware_filename) {
  struct stat sb;
  if (stat(firmware_filename, &sb) == -1) {
    return false;
  }
  uint8_t* blob = new uint8_t[sb.st_size];
  FILE* f = fopen(firmware_filename, "rb");
  auto result = fread(blob, 1, sb.st_size, f);
  if (result != sb.st_size) {
    printf("fread failed\n");
    exit(-1);
  }
  fclose(f);

  Elf32_Ehdr& header = *(Elf32_Ehdr*)blob;
  for (int i = 0; i < header.e_phnum; i++) {
    Elf32_Phdr& phdr = *(Elf32_Phdr*)(blob + header.e_phoff + header.e_phentsize * i);
    if (phdr.p_type & PT_LOAD) {
      if (phdr.p_flags & PF_X) {
        LOG_G("Code @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);
        //int len = code_ram.data_size() < phdr.p_filesz ? code_ram.data_size() : phdr.p_filesz;
        //memcpy(code_ram.get_data(), blob + phdr.p_offset, len);
      }
      else if (phdr.p_flags & PF_W) {
        LOG_G("Data @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);
        //int len = data_ram.data_size() < phdr.p_filesz ? data_ram.data_size() : phdr.p_filesz;
        //memcpy(data_ram.get_data(), blob + phdr.p_offset, len);
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------

TestResults run_test_elf(const char* test_filename, int reps = 1, int max_cycles = 100000, bool expect_fail = false) {
  TEST_INIT("'%-30s', %d reps: ", test_filename, reps);

  double time = 0;
  int tocks = 0;
  double time_a, time_b;
  int elapsed_cycles = 0;

  load_elf(test_filename);

  pinwheel top("code.hex", "data.hex");

  bool load_ok = top.load_elf(test_filename);
  if (!load_ok) {
    TEST_FAIL("Could not load %s\n", test_filename);
  }

  for (int rep = 0; rep < reps; rep++) {
    top.tock(1, 0, 0);
    top.tick(1, 0, 0);
    double time_a = timestamp();
    for (elapsed_cycles = 0; elapsed_cycles < max_cycles; elapsed_cycles++) {
      top.tock(0, 0, 0);
      top.tick(0, 0, 0);
      tocks++;

      // logic<3>  a_opcode;
      // logic<3>  a_param;
      // logic<3>  a_size;
      // logic<1>  a_source;
      // logic<32> a_address;

      if (top.core.bus_tla.a_address == 0xFFFFFFF0 && top.core.bus_tla.a_opcode == TL::PutFullData) {
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
    char firmware_filename[256];
    sprintf(firmware_filename, "tests/rv_tests/%s.elf", instructions[i]);
    results << run_test_elf(firmware_filename, reps, max_cycles);
  }
  TEST_DONE();
}

//------------------------------------------------------------------------------

TestResults run_microtests() {
  TEST_INIT("Running microtests");
  results << run_test_elf("bin/tests/firmware/basic");
  results << run_test_elf("bin/tests/firmware/call_jalr");
  results << run_test_elf("bin/tests/firmware/get_hart");
  results << run_test_elf("bin/tests/firmware/start_thread");
  results << run_test_elf("bin/tests/firmware/stepping");
  results << run_test_elf("bin/tests/firmware/write_regs");
  results << run_test_elf("bin/tests/firmware/yield");
  results << run_test_elf("bin/tests/firmware/read_regs");

  // This one is broken...?
  //results << run_test_elf("bin/tests/firmware/write_code");
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
  results << run_microtests();
  results.show_result();

  LOG_B("Total tocks %f\n", double(total_tocks));
  LOG_B("Total time %f\n", double(total_time));
  double rate = double(total_tocks) / double(total_time);
  LOG_G("Sim rate %f mhz\n", rate / 1.0e6);

  return results.test_fail ? 1 : 0;
}

//------------------------------------------------------------------------------
