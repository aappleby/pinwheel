#pragma once
#include "metron_tools.h"

#include "constants.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal

class regfile {
public:

  regfile() {
    for (int i = 0; i < 1024; i++) data[i] = 0;
  }

  void tick(logic<10> raddr1 , logic<10> raddr2, logic<10> waddr, logic<32> wdata, logic<1> wren) {
    out_1 = data[raddr1];
    out_2 = data[raddr2];

    if (wren) {
      data[waddr] = wdata;
    }

    if (wren && raddr1 == waddr) out_1 = wdata;
    if (wren && raddr2 == waddr) out_2 = wdata;
  }

  logic<32> get_rs1() const { return out_1; }
  logic<32> get_rs2() const { return out_2; }

  // metron_noconvert
  const uint32_t* get_data() const {
    return (uint32_t*)data;
  }

  // metron_internal
  logic<32> data[1024];
  logic<32> out_1;
  logic<32> out_2;
};

//------------------------------------------------------------------------------
// verilator lint_on unusedsignal
