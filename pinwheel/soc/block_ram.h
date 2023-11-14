#ifndef PINWHEEL_RTL_BLOCK_RAM_H
#define PINWHEEL_RTL_BLOCK_RAM_H

#include "metron/metron_tools.h"
#include "pinwheel/tools/tilelink.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off unusedparam

template <uint32_t addr_mask = 0xF0000000, uint32_t addr_tag = 0x00000000>
class block_ram {
public:

  block_ram(const char* filename = nullptr) {
    if (filename) {
      readmemh(filename, data);
    }
    bus_tld.d_opcode = b3(DONTCARE);
    bus_tld.d_param  = b2(DONTCARE);
    bus_tld.d_size   = b3(DONTCARE);
    bus_tld.d_source = b1(DONTCARE);
    bus_tld.d_sink   = b3(DONTCARE);
    bus_tld.d_data   = b32(DONTCARE);
    bus_tld.d_error  = 0;
    bus_tld.d_valid  = 0;
    bus_tld.d_ready  = 1;
  }

  /* metron_noconvert */ logic<32> get() const { return bus_tld.d_data; }

  void tock(tilelink_a tla) {
    tick(tla);
  }

  /* metron_noconvert */ uint32_t* get_data() { return (uint32_t*)data; }
  /* metron_noconvert */ size_t data_size() const { return sizeof(data); }
  /* metron_noconvert */ const uint32_t* get_data() const { return (uint32_t*)data; }

  tilelink_d bus_tld;

private:

  void tick(tilelink_a tla) {
    bus_tld.d_opcode = b3(DONTCARE);
    bus_tld.d_param  = b2(DONTCARE);
    bus_tld.d_size   = b3(DONTCARE);
    bus_tld.d_source = b1(DONTCARE);
    bus_tld.d_sink   = b3(DONTCARE);
    bus_tld.d_data   = b32(DONTCARE);
    bus_tld.d_error  = 0;
    bus_tld.d_valid  = 0;
    bus_tld.d_ready  = 1;

    if (tla.a_valid && ((tla.a_address & addr_mask) == addr_tag)) {
      if (tla.a_opcode == TL::PutPartialData) {
        logic<32> old_data = data[b14(tla.a_address, 2)];
        logic<32> new_data = tla.a_data;
        if (tla.a_address[0]) new_data = new_data << 8;
        if (tla.a_address[1]) new_data = new_data << 16;
        new_data = ((tla.a_mask[0] ? new_data : old_data) & 0x000000FF) |
                  ((tla.a_mask[1] ? new_data : old_data) & 0x0000FF00) |
                  ((tla.a_mask[2] ? new_data : old_data) & 0x00FF0000) |
                  ((tla.a_mask[3] ? new_data : old_data) & 0xFF000000);

        data[b14(tla.a_address, 2)] = new_data;
        bus_tld.d_opcode = TL::AccessAckData;
        bus_tld.d_data = new_data;
        bus_tld.d_valid = 1;
      }
      else if (tla.a_opcode == TL::PutFullData) {
        data[b14(tla.a_address, 2)] = tla.a_data;
        bus_tld.d_opcode = TL::AccessAckData;
        bus_tld.d_data = tla.a_data;
        bus_tld.d_valid = 1;
      }
      else if (tla.a_opcode == TL::Get) {
        bus_tld.d_opcode = TL::AccessAckData;
        bus_tld.d_data = data[b14(tla.a_address, 2)];
        bus_tld.d_valid = 1;
      }
    }
  }

  logic<32> data[16384];
};

// verilator lint_on unusedsignal
// verilator lint_on unusedparam
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_BLOCK_RAM_H
