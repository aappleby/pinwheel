#include "pinwheel/soc/pinwheel_soc.h"
#include "metrolib/core/Log.h"

#include <elf.h>
#include <sys/stat.h>

pinwheel* pinwheel::clone() {
  pinwheel* p = new pinwheel(nullptr, nullptr, nullptr);
  memcpy(p, this, sizeof(*this));
  return p;
}

size_t pinwheel::size_bytes() { return sizeof(*this); }

uint32_t* pinwheel::get_code() { return code_ram.get_data(); }

uint32_t* pinwheel::get_data() { return data_ram.get_data(); }

logic<32> pinwheel::get_debug() const { return debug_reg.get(); }

bool pinwheel::load_elf(const char* firmware_filename) {
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
        int len = code_ram.data_size() < phdr.p_filesz ? code_ram.data_size() : phdr.p_filesz;
        memcpy(code_ram.get_data(), blob + phdr.p_offset, len);
      }
      else if (phdr.p_flags & PF_W) {
        LOG_G("Data @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);
        int len = data_ram.data_size() < phdr.p_filesz ? data_ram.data_size() : phdr.p_filesz;
        memcpy(data_ram.get_data(), blob + phdr.p_offset, len);
      }
    }
  }
  return true;
}
