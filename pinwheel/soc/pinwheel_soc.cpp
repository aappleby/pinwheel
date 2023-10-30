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
