#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <elf.h>
#include <sys/stat.h>
//#include <arch/riscv/include/asm/elf.h>

#include "Tests.h"
#include "pinwheel.h"
#include "pinwheel_app.h"

#include "AppLib/AppHost.h"

//------------------------------------------------------------------------------

int main(int argc, const char** argv) {

  PinwheelApp app;
  AppHost host(&app);

#if 1

  const char* firmware_filename = "firmware/bin/hello";

  LOG_G("Loading firmware %s...\n", firmware_filename);
  struct stat sb;
  if (stat(firmware_filename, &sb) == -1) {
    LOG_R("Could not stat firmware %s\n", firmware_filename);
    return 0;
  }
  LOG_G("Firmware is %d bytes\n", sb.st_size);
  uint8_t* blob = new uint8_t[sb.st_size];
  FILE* f = fopen(firmware_filename, "rb");
  auto result = fread(blob, 1, sb.st_size, f);
  fclose(f);

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
        LOG_G("Code @ %p = %d bytes\n",blob + phdr.p_offset, phdr.p_filesz);
        put_cache("rv_tests/test_elf.text.vh", blob + phdr.p_offset, phdr.p_filesz);
      }
      if (phdr.p_vaddr == 0x80000000) {
        LOG_G("Data @ %p = %d bytes\n",blob + phdr.p_offset, phdr.p_filesz);
        put_cache("rv_tests/test_elf.data.vh", blob + phdr.p_offset, phdr.p_filesz);
      }
    }
  }

#endif

  auto app_result = host.app_main(argc, argv);

#if 0


  test_elf(blob, sb.st_size, 1, 1000000000);

  /*
  int reps = 1000;
  int max_cycles = 10000;

  LOG_B("Starting %s @ %d reps...\n", argv[0], reps);

  total_tocks = 0;
  total_time = 0;

  TestResults results;
  results += test_instructions(reps, max_cycles);
  results.dump();


  return results.test_fail ? 0 : 1;
  */

  LOG_B("Total tocks %f\n", double(total_tocks));
  LOG_B("Total time %f\n", double(total_time));
  double rate = double(total_tocks) / double(total_time);
  LOG_G("Sim rate %f mhz\n", rate / 1.0e6);

  delete [] blob;
#endif

  printf("main() done\n");
  return app_result;
}

//------------------------------------------------------------------------------
