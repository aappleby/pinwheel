#ifndef PINWHEEL_SOC_BLOCK_RAM_H
#define PINWHEEL_SOC_BLOCK_RAM_H

// verilator lint_off unusedsignal
// verilator lint_off unusedparam

#include "metron/metron_tools.h"

//------------------------------------------------------------------------------

template<int width, int depth>
class block_ram {
public:

  static const int addr_bits = clog2(depth);

  block_ram(const char* filename = nullptr) {
    if (filename) {
      readmemh(filename, data);
    }
  }

  void tock(logic<addr_bits> raddr, logic<addr_bits> waddr, logic<width> wdata, logic<1> wren) {
    tick(raddr, waddr, wdata, wren);
  }

  logic<width> get_data() { return out; }

  /* metron_noconvert */ uint32_t* get_data() { return (uint32_t*)data; }
  /* metron_noconvert */ size_t data_size() const { return sizeof(data); }
  /* metron_noconvert */ const uint32_t* get_data() const { return (uint32_t*)data; }

private:

  void tick(logic<addr_bits> raddr, logic<addr_bits> waddr, logic<width> wdata, logic<1> wren) {
    if (wren) {
      out = raddr == waddr ? wdata : data[raddr];
      data[waddr] = wdata;
    }
    else {
      out = data[raddr];
    }
  }

  logic<width> data[depth];
  logic<width> out;
};

//------------------------------------------------------------------------------

// verilator lint_on unusedsignal
// verilator lint_on unusedparam

#endif // PINWHEEL_SOC_BLOCK_RAM_H
