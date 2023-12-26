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

  block_ram(const char* filename = "pinwheel/uart/message.hex") {
    if (filename) {
      readmemh(filename, ram_);
    }
  }

  void tock(logic<12> addr, logic<16> wdata, logic<1> wren) {
    if (wren) {
      ram_[addr] = wdata;
    }
    else {
      rdata_ = ram_[addr];
    }
  }

  logic<16> rdata_;

private:

  logic<16> ram_[dwords];
};

//------------------------------------------------------------------------------

class top {
public:

  top() : block_ram("pinwheel/uart/message.hex") {
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

//------------------------------------------------------------------------------

// verilator lint_on unusedsignal
// verilator lint_on unusedparam

#endif // PINWHEEL_SOC_BLOCK_RAM_H
