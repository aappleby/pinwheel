#ifndef PINWHEEL_SOC_BUS_RAM_H
#define PINWHEEL_SOC_BUS_RAM_H

// verilator lint_off unusedsignal
// verilator lint_off unusedparam

#include "metron/metron_tools.h"
#include "pinwheel/tools/tilelink.h"
#include "pinwheel/soc/block_ram.h"

// FIXME we are intentionally ignoring the tilelink 'ready' signals



//------------------------------------------------------------------------------

template <uint32_t addr_mask = 0xF0000000, uint32_t addr_tag = 0x00000000, int dwords = 16384>
class bus_ram {
public:

  static const int addr_bits = clog2(dwords);

  bus_ram(const char* filename = nullptr) : ram(filename) {
    tld_.d_opcode = b32(DONTCARE);
    tld_.d_param  = b2(DONTCARE);
    tld_.d_size   = b3(DONTCARE);
    tld_.d_source = b1(DONTCARE);
    tld_.d_sink   = b3(DONTCARE);
    tld_.d_data   = b32(DONTCARE);
    tld_.d_error  = b1(DONTCARE);
    tld_.d_valid  = 0;
    tld_.d_ready  = 1;
  }

  /* metron_noconvert */ logic<32> get() const { return tld_.d_data; }

  //----------------------------------------

  tilelink_d get_tld() {
    tilelink_d result;

    result.d_opcode = tld_.d_opcode;
    result.d_param  = tld_.d_param;
    result.d_size   = tld_.d_size;
    result.d_source = tld_.d_source;
    result.d_sink   = tld_.d_sink;
    result.d_data   = ram.rdata_;     // Route the ram output from the _previous_ cycle to TLD
    result.d_error  = tld_.d_error;
    result.d_valid  = tld_.d_valid;
    result.d_ready  = tld_.d_ready;

    return result;
  }

  void tock_b(tilelink_a tla) {
    logic<1> cs   = tla.a_valid && ((tla.a_address & addr_mask) == addr_tag);
    logic<1> wren = (tla.a_opcode == TL::PutFullData) || (tla.a_opcode == TL::PutPartialData);

    ram.tock(cs, bx<addr_bits>(tla.a_address, 2), tla.a_data, wren, tla.a_mask);

    tick(cs, tla);
  }

  //----------------------------------------

  /* metron_noconvert */ const uint32_t* get_data() const { return ram.get_data(); }
  /* metron_noconvert */ uint32_t*       get_data()       { return ram.get_data(); }
  /* metron_noconvert */ size_t          get_size() const { return ram.get_size(); }

private:

  //----------------------------------------

  void tick(logic<1> cs, tilelink_a tla) {
    tld_.d_opcode = TL::Invalid;
    tld_.d_size   = tla.a_size;
    tld_.d_valid  = 0;

    if (cs) {
      if (tla.a_opcode == TL::PutFullData || tla.a_opcode == TL::PutPartialData) {
        tld_.d_opcode = TL::AccessAck;
        tld_.d_valid  = 1;
      }

      if (tla.a_opcode == TL::Get) {
        tld_.d_opcode = TL::AccessAckData;
        tld_.d_valid  = 1;
      }
    }
  }

  //----------------------------------------

  tilelink_d tld_;
  block_ram<dwords> ram;
};

//------------------------------------------------------------------------------

// verilator lint_on unusedsignal
// verilator lint_on unusedparam

#endif // PINWHEEL_SOC_BUS_RAM_H
