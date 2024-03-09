#ifndef PINWHEEL_SOC_BLOCK_RAM_H
#define PINWHEEL_SOC_BLOCK_RAM_H

// verilator lint_off unusedsignal
// verilator lint_off unusedparam

#include "metron/metron_tools.h"

//FIXME this should be using SB_SPRAM256KA not SB_RAM40_4K

//------------------------------------------------------------------------------

template<int dwords = 4096>
class block_ram {
public:

  static const int addr_bits = clog2(dwords);

  block_ram(const char* filename = "pinwheel/uart/message.hex") {
    if (filename) {
      readmemh(filename, ram_);
    }
  }

  void tock(logic<addr_bits> addr, logic<32> wdata, logic<4> wmask) {
    if (wmask) {
      rdata_ = ram_[addr];

      logic<32> mask = 0x00000000;
      if (wmask[0]) mask = mask | 0x000000FF;
      if (wmask[1]) mask = mask | 0x0000FF00;
      if (wmask[2]) mask = mask | 0x00FF0000;
      if (wmask[3]) mask = mask | 0xFF000000;

      ram_[addr] = (wdata & mask) | (rdata_ & ~mask);
    }
    else {
      rdata_ = ram_[addr];
    }
  }

  logic<32> rdata_;

  /* metron_noconvert */ const uint32_t* get_data() const { return (const uint32_t*)ram_; }
  /* metron_noconvert */ uint32_t*       get_data()       { return (uint32_t*)ram_; }
  /* metron_noconvert */ size_t          get_size() const { return dwords * 4; }

private:

  logic<32> ram_[dwords];
};

//------------------------------------------------------------------------------

#if 0
class top {
public:

  top() : my_ram("pinwheel/uart/message.hex") {
    counter_ = 0;
  }

  logic<16> get_data() {
    return my_ram.rdata_;
  }

  void tock(logic<1> wren) {
    my_ram.tock(b12(counter_), b16(counter_), wren);
  }

private:
  logic<32> counter_;

  block_ram<4096> my_ram;
};
#endif

//------------------------------------------------------------------------------

// verilator lint_on unusedsignal
// verilator lint_on unusedparam

#endif // PINWHEEL_SOC_BLOCK_RAM_H
