#ifndef PINWHEEL_SOC_BLOCK_RAM_H
#define PINWHEEL_SOC_BLOCK_RAM_H

// verilator lint_off unusedsignal
// verilator lint_off unusedparam

#include "metron/metron_tools.h"

//------------------------------------------------------------------------------

template<int dwords>
class block_ram {
public:

  static const int addr_bits = clog2(dwords);

  block_ram(const char* filename = nullptr) {
    if (filename) {
      readmemh(filename, ram);
    }
  }

  void tock(logic<1> cs, logic<addr_bits> addr, logic<32> wdata, logic<1> wren, logic<4> mask) {
    tick(cs, addr, wdata, wren, mask);
  }

  logic<32> rdata;

  /* metron_noconvert */ const uint32_t* get_data() const { return (const uint32_t*)ram; }
  /* metron_noconvert */ uint32_t*       get_data()       { return (uint32_t*)ram; }
  /* metron_noconvert */ size_t          get_size() const { return sizeof(ram); }

private:

  void tick(logic<1> cs, logic<addr_bits> addr, logic<32> wdata, logic<1> wren, logic<4> mask) {
    if (cs) {
      if (wren) {
        if (mask[0]) slice< 7,  0, 32>(ram[addr]) = b8(wdata,  0);
        if (mask[1]) slice<15,  8, 32>(ram[addr]) = b8(wdata,  8);
        if (mask[2]) slice<23, 16, 32>(ram[addr]) = b8(wdata, 16);
        if (mask[3]) slice<31, 24, 32>(ram[addr]) = b8(wdata, 24);
      }
      else {
        rdata = ram[addr];
      }
    }
  }

  logic<32> ram[dwords];
};

//------------------------------------------------------------------------------

// verilator lint_on unusedsignal
// verilator lint_on unusedparam

#endif // PINWHEEL_SOC_BLOCK_RAM_H
