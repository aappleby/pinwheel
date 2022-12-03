#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <elf.h>
#include <sys/stat.h>
//#include <arch/riscv/include/asm/elf.h>

#include "Tests.h"
#include "pinwheel.h"
#include "pinwheel_demo.h"

#include "AppLib/AppHost.h"

//------------------------------------------------------------------------------

int main(int argc, const char** argv) {
  PinwheelApp app;
  AppHost host(&app);

  int result = host.app_main(argc, argv);

  printf("main()\n");
  return result;

#if 0

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

  return 0;
}

//------------------------------------------------------------------------------
