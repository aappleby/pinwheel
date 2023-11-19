#ifndef PINWHEEL_SOC_BUS_RAM_H
#define PINWHEEL_SOC_BUS_RAM_H

// verilator lint_off unusedsignal
// verilator lint_off unusedparam

#include "metron/metron_tools.h"
#include "pinwheel/tools/tilelink.h"
#include "pinwheel/soc/block_ram.h"

// FIXME we are intentionally ignoring the tilelink 'ready' signals

//------------------------------------------------------------------------------

template <uint32_t addr_mask = 0xF0000000, uint32_t addr_tag = 0x00000000>
class bus_ram {
public:

  bus_ram(const char* filename = nullptr) : ram(filename) {
    tld.d_opcode = TL::Invalid;
    tld.d_param  = b2(DONTCARE);
    tld.d_size   = b3(DONTCARE);
    tld.d_source = b1(DONTCARE);
    tld.d_sink   = b3(DONTCARE);
    tld.d_data   = b32(DONTCARE);
    tld.d_error  = 0;
    tld.d_valid  = 0;
    tld.d_ready  = 1;
  }

  /* metron_noconvert */ logic<32> get() const { return tld.d_data; }

  //----------------------------------------

  tilelink_d get_tld() {
    // Route the ram output from the _previous_ cycle to TLD
    tld.d_data = ram.rdata;
    return tld;
  }

  void tock_b(tilelink_a tla) {
    logic<1> cs   = tla.a_valid && ((tla.a_address & addr_mask) == addr_tag);
    logic<1> wren = (tla.a_opcode == TL::PutFullData) || (tla.a_opcode == TL::PutPartialData);

    ram.tock(cs, b11(tla.a_address, 2), tla.a_data, wren, tla.a_mask);

    tick(cs, tla);
  }

  //----------------------------------------

  void tick(logic<1> cs, tilelink_a tla) {
    if (cs) {
      switch(tla.a_opcode) {
        case TL::PutFullData:
          tld.d_opcode = TL::AccessAck;
          tld.d_size   = tla.a_size;
          tld.d_valid  = 1;
          break;
        case TL::PutPartialData:
          tld.d_opcode = TL::AccessAck;
          tld.d_size   = tla.a_size;
          tld.d_valid  = 1;
          break;
        case TL::Get:
          tld.d_opcode = TL::AccessAckData;
          tld.d_size   = tla.a_size;
          tld.d_valid  = 1;
          break;
        default:
          tld.d_opcode = b3(DONTCARE);
          tld.d_size   = b3(DONTCARE);
          tld.d_valid  = 0;
          break;
      }
    }
    else {
      tld.d_opcode = TL::Invalid;
      tld.d_valid  = 0;
    }
  }

  //----------------------------------------

  /* metron_noconvert */ const uint32_t* get_data() const { return ram.get_data(); }
  /* metron_noconvert */ uint32_t*       get_data()       { return ram.get_data(); }
  /* metron_noconvert */ size_t          get_size() const { return ram.get_size(); }

private:

  tilelink_d tld;

  // 2048 dwords, 8k bytes
  block_ram<2048> ram;
};

//------------------------------------------------------------------------------

// verilator lint_on unusedsignal
// verilator lint_on unusedparam

#endif // PINWHEEL_SOC_BUS_RAM_H
