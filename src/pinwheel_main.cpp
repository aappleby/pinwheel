#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <elf.h>
#include <sys/stat.h>
//#include <arch/riscv/include/asm/elf.h>

#include "CoreLib/Tests.h"
#include "pinwheel.h"
#include "pinwheel_app.h"

#include "AppLib/AppHost.h"

//------------------------------------------------------------------------------

int main(int argc, const char** argv) {

  PinwheelApp app;
  AppHost host(&app);

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
    "+text_file=rv_tests/firmware.text.vh",
    "+data_file=rv_tests/firmware.data.vh"
  };
  metron_init(2, argv2);

  Elf32_Ehdr& header = *(Elf32_Ehdr*)blob;

  app.pinwheel_sim->states.top().vane0_pc = header.e_entry;

  for (int i = 0; i < header.e_phnum; i++) {
    Elf32_Phdr& phdr = *(Elf32_Phdr*)(blob + header.e_phoff + header.e_phentsize * i);
    if (phdr.p_type & PT_LOAD) {
      if (phdr.p_flags & PF_X) {
        LOG_G("Code @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);
        put_cache("rv_tests/firmware.text.vh", blob + phdr.p_offset, phdr.p_filesz);
      }
      else if (phdr.p_flags & PF_W) {
        LOG_G("Data @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);
        put_cache("rv_tests/firmware.data.vh", blob + phdr.p_offset, phdr.p_filesz);
      }
    }
  }

  auto app_result = host.app_main(argc, argv);

  printf("main() done\n");
  return app_result;
}

//------------------------------------------------------------------------------
