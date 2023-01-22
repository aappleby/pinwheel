// Serial device with a TL-UL interface

#pragma once
#include "metron_tools.h"
#include "tilelink.h"

// verilator lint_off unusedsignal
// verilator lint_off undriven

class Serial {
public:
  tilelink_a tla;
  tilelink_d tld;

  /*
  // Get
  a_opcode = 4;
  a_param;
  a_size;
  a_source;
  a_address;
  a_mask;
  a_data = b32(DONTCARE);
  a_valid;
  a_ready;
  */

  /*
  // PutFullData
  a_opcode = 0;
  a_param = 0;
  a_size;
  a_source;
  a_address;
  a_mask;
  a_data;
  a_valid;
  a_ready;
  */

  /*
  // PutPartialData
  a_opcode = 1;
  a_param = 0;
  a_size;
  a_source;
  a_address;
  a_mask;
  a_data;
  a_valid;
  a_ready;
  */

  void tock() {
  }

private:

  void tick() {
    tld.d_size = tla.a_size;

    if (tla.a_opcode == TL::Get) {
      tld.d_opcode = TL::AccessAckData;
      tld.d_param  = 0;
      tld.d_size   = tla.a_size;
      tld.d_source = tla.a_source;
      tld.d_sink   = b3(DONTCARE);
      tld.d_data   = b32(0xDEADBEEF);
      tld.d_error  = 0;
      tld.d_valid  = 1;
      tld.d_ready  = 1;
    }
    else if (tla.a_opcode == TL::PutFullData) {
      tld.d_opcode = TL::AccessAck;
      tld.d_param  = 0;
      tld.d_size   = tla.a_size;
      tld.d_source = tla.a_source;
      tld.d_sink   = 0;
      tld.d_data   = b32(DONTCARE);
      tld.d_error  = 0;
      tld.d_valid  = 0;
      tld.d_ready  = 1;
    }
    else if (tla.a_opcode == TL::PutPartialData) {
      tld.d_opcode = TL::AccessAck;
      tld.d_param  = 0;
      tld.d_size   = tla.a_size;
      tld.d_source = tla.a_source;
      tld.d_sink   = 0;
      tld.d_data   = b32(DONTCARE);
      tld.d_error  = 0;
      tld.d_valid  = 0;
      tld.d_ready  = 1;
    }
  }
};

// verilator lint_on unusedsignal
// verilator lint_on undriven
