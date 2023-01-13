#include "pinwheel.h"

#include <elf.h>
#include <sys/stat.h>

bool pinwheel::load_elf(const char* firmware_filename) {
  struct stat sb;
  if (stat(firmware_filename, &sb) == -1) {
    return false;
  }
  uint8_t* blob = new uint8_t[sb.st_size];
  FILE* f = fopen(firmware_filename, "rb");
  auto result = fread(blob, 1, sb.st_size, f);
  fclose(f);

  Elf32_Ehdr& header = *(Elf32_Ehdr*)blob;
  for (int i = 0; i < header.e_phnum; i++) {
    Elf32_Phdr& phdr = *(Elf32_Phdr*)(blob + header.e_phoff + header.e_phentsize * i);
    if (phdr.p_type & PT_LOAD) {
      if (phdr.p_flags & PF_X) {
        int len = sizeof(code.data) < phdr.p_filesz ? sizeof(code.data) : phdr.p_filesz;
        memcpy(code.data, blob + phdr.p_offset, len);
      }
      else if (phdr.p_flags & PF_W) {
        int len = sizeof(data_ram.data) < phdr.p_filesz ? sizeof(data_ram.data) : phdr.p_filesz;
        memcpy(data_ram.data, blob + phdr.p_offset, len);
      }
    }
  }
  return true;
}
