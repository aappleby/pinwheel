#include "metrolib/core/Log.h"

#include <stdio.h>
#include <elf.h>
#include <sys/stat.h>
#include <filesystem>
#include <string>
#include <vector>
#include <stdint.h>

namespace fs = std::filesystem;

std::vector<uint8_t> code;
std::vector<uint8_t> data;

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
    LOG_R("fread failed\n");
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

int main(int argc, char** argv) {
  if (argc < 2) {
    LOG_R("Usage : elf_to_hex <elf_filename>\n");
    return -1;
  }

  std::string elf_path = argv[1];
  if (!fs::is_regular_file(elf_path)) {
    LOG_R("File %s not found\n", elf_path.c_str());
    return -1;
  }

  //fs::path absolute_dir(elf_path);
  //absolute_dir.remove_filename();

  return 0;
}

//------------------------------------------------------------------------------
