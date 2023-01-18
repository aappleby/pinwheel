#pragma once
#include "metron_tools.h"

#include "constants.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal

class regfile {
public:

  regfile() {
    for (int i = 0; i < 256; i++) {
      data1_hi[i] = 0;
      data1_lo[i] = 0;
      data2_hi[i] = 0;
      data2_lo[i] = 0;
    }
  }

  void tick(logic<8> raddr1 , logic<8> raddr2, logic<8> waddr, logic<32> wdata, logic<1> wren) {
    if (wren) {
      out_1 = raddr1 == waddr ? wdata : cat(data1_hi[raddr1], data1_lo[raddr1]);
      out_2 = raddr2 == waddr ? wdata : cat(data2_hi[raddr2], data2_lo[raddr2]);
      data1_hi[waddr] = b16(wdata, 16);
      data1_lo[waddr] = b16(wdata, 0);
      data2_hi[waddr] = b16(wdata, 16);
      data2_lo[waddr] = b16(wdata, 0);
    }
    else {
      out_1 = cat(data1_hi[raddr1], data1_lo[raddr1]);
      out_2 = cat(data2_hi[raddr2], data2_lo[raddr2]);
    }
  }

  logic<32> get_rs1() const { return out_1; }
  logic<32> get_rs2() const { return out_2; }

  // metron_noconvert
  const uint32_t get(int index) const {
    return cat(data1_hi[index], data1_lo[index]);
  }

  // metron_internal
  logic<16> data1_hi[256];
  logic<16> data1_lo[256];
  logic<16> data2_hi[256];
  logic<16> data2_lo[256];
  logic<32> out_1;
  logic<32> out_2;
};

//------------------------------------------------------------------------------
// verilator lint_on unusedsignal
