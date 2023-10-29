#include <stdio.h>
#include <elf.h>
#include <sys/stat.h>
#include <filesystem>
#include <string>
#include <vector>
#include <stdint.h>
#include <assert.h>

namespace fs = std::filesystem;

std::vector<uint8_t> code;
std::vector<uint8_t> data;

/*
Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  LOAD           0x001000 0x00400000 0x00400000 0x004a4 0x004a4 R E 0x1000
  LOAD           0x002000 0x80000000 0x80000000 0x00028 0x00028 RW  0x1000
  NULL           0x000000 0x80000028 0x80000028 0x00000 0x00800 RW  0x1
*/

//------------------------------------------------------------------------------

bool load_elf(std::string filename) {
  auto file_size = fs::file_size(filename);
  fs::path file_path(filename);

  auto code_hex_path = file_path;
  auto data_hex_path = file_path;
  code_hex_path.replace_extension(".code.vh");
  data_hex_path.replace_extension(".data.vh");

  //printf("ELF file %s = %ld bytes\n", filename.c_str(), file_size);
  //printf("code path %s\n", code_hex_path.c_str());
  //printf("data path %s\n", data_hex_path.c_str());

  uint8_t* blob = new uint8_t[file_size];
  FILE* f = fopen(filename.c_str(), "rb");
  auto result = fread(blob, 1, file_size, f);
  if (result != file_size) {
    printf("fread failed\n");
    exit(-1);
  }
  fclose(f);

  bool found_code = false;
  bool found_data = false;

  Elf32_Ehdr& header = *(Elf32_Ehdr*)blob;
  for (int i = 0; i < header.e_phnum; i++) {
    Elf32_Phdr& phdr = *(Elf32_Phdr*)(blob + header.e_phoff + header.e_phentsize * i);
    if (phdr.p_type & PT_LOAD) {
      assert(phdr.p_filesz % 4 == 0);

      if (phdr.p_flags & PF_X) {
        assert(!found_code);
        found_code = true;
        //printf("Code @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);

        uint32_t* words = (uint32_t*)(blob + phdr.p_offset);
        int word_count = phdr.p_filesz / 4;

        FILE* out = fopen(code_hex_path.c_str(), "w");
        for (int i = 0; i < word_count; i++) {
          fprintf(out, "%08X ", words[i]);
          if (i % 8 == 7) fprintf(out, "\n");
        }
        fprintf(out, "\n");
        fclose(out);
      }
      else if (phdr.p_flags & PF_W) {
        assert(!found_data);
        found_data = true;
        //printf("Data @ 0x%08x = %d bytes\n", phdr.p_vaddr, phdr.p_filesz);

        uint32_t* words = (uint32_t*)(blob + phdr.p_offset);
        int word_count = phdr.p_filesz / 4;

        FILE* out = fopen(data_hex_path.c_str(), "w");
        for (int i = 0; i < word_count; i++) {
          fprintf(out, "%08X ", words[i]);
          if (i % 8 == 7) fprintf(out, "\n");
        }
        fprintf(out, "\n");
        fclose(out);
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage : elf_to_hex <elf_filename>\n");
    return -1;
  }

  std::string elf_path = argv[1];
  if (!fs::is_regular_file(elf_path)) {
    printf("File %s not found\n", elf_path.c_str());
    return -1;
  }

  load_elf(elf_path);

  //fs::path absolute_dir(elf_path);
  //absolute_dir.remove_filename();

  return 0;
}

//------------------------------------------------------------------------------
