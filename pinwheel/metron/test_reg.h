#ifndef PINWHEEL_RTL_TEST_REG_H
#define PINWHEEL_RTL_TEST_REG_H

#include "metron/metron_tools.h"
#include "pinwheel/metron/tilelink.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off unusedparam

template <uint32_t addr_mask = 0xF0000000, uint32_t addr_tag = 0xF0000000>
class test_reg {
public:

  tilelink_d bus_tld;

  /* metron_noconvert */ logic<32> get() const { return bus_tld.d_data; }

  test_reg(logic<32> init = 0) {
    bus_tld.d_opcode = TL::AccessAckData;
    bus_tld.d_param  = b2(DONTCARE);
    bus_tld.d_size   = b3(DONTCARE);
    bus_tld.d_source = b1(DONTCARE);
    bus_tld.d_sink   = b3(DONTCARE);
    bus_tld.d_data   = init;
    bus_tld.d_error  = 0;
    bus_tld.d_valid  = 0;
    bus_tld.d_ready  = 1;
  }

  void tick(tilelink_a tla) {
    logic<1> cs = tla.a_valid && ((tla.a_address & addr_mask) == addr_tag);

    bus_tld.d_valid = cs;

    if (cs && ((tla.a_opcode == TL::PutPartialData) || (tla.a_opcode == TL::PutFullData))) {
      logic<32> mask = expand_bitmask(tla.a_mask);
      bus_tld.d_data = (bus_tld.d_data & ~mask) | (tla.a_data & mask);
    }
  }

private:

  logic<32> expand_bitmask(logic<4> mask) {
    return cat(dup<8>(mask[3]), dup<8>(mask[2]), dup<8>(mask[1]), dup<8>(mask[0]));
  }
};

// verilator lint_on unusedsignal
// verilator lint_off unusedparam
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_TEST_REG_H
