#ifndef PINWHEEL_RTL_SERIAL_H
#define PINWHEEL_RTL_SERIAL_H

// Serial device with a TL-UL interface

//------------------------------------------------------------------------------

#include "metron/metron_tools.h"
#include "pinwheel/metron/tilelink.h"

// verilator lint_off unusedsignal
// verilator lint_off undriven

class Serial {
public:
  tilelink_a tla;
  tilelink_d tld;

  void tock() {
    tick();
  }

private:

  logic<32> test_reg;

  void tick() {
    tld.d_param  = 0;
    tld.d_size   = tla.a_size;
    tld.d_source = tla.a_source;
    tld.d_sink   = b3(DONTCARE);
    tld.d_data   = b32(DONTCARE);
    tld.d_error  = 0;
    tld.d_valid  = 0;
    tld.d_ready  = 1;

    if (tla.a_opcode == TL::Get) {
      tld.d_opcode = TL::AccessAckData;
      if (b4(tla.a_address, 28) == 0x5) {
        tld.d_data   = test_reg;
        tld.d_valid  = 1;
      }
    }
    else if (tla.a_opcode == TL::PutFullData || tla.a_opcode == TL::PutPartialData) {
      tld.d_opcode = TL::AccessAck;
      logic<32> bitmask = expand_bitmask(tla.a_mask);
      test_reg = (test_reg & ~bitmask) | (tla.a_data & bitmask);
    }
  }

  logic<32> expand_bitmask(logic<4> mask) {
    return cat(dup<8>(mask[3]), dup<8>(mask[2]), dup<8>(mask[1]), dup<8>(mask[0]));
  }

};

// verilator lint_on unusedsignal
// verilator lint_on undriven

//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_SERIAL_H
