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

  void tock(logic<10> raddr1_ , logic<10> raddr2_, logic<10> waddr_, logic<32> wdata_, logic<1> wren_) {
    raddr1 = raddr1_;
    raddr2 = raddr2_;
    waddr  = waddr_;
    wdata  = wdata_;
    wren   = wren_;
  }

  void tick() {
    out_1 = data[raddr1];
    out_2 = data[raddr2];

    if (wren) data[waddr] = wdata;

    if (wren && raddr1 == waddr) out_1 = wdata;
    if (wren && raddr2 == waddr) out_2 = wdata;
  }

  logic<32> get_rs1() const { return out_1; }
  logic<32> get_rs2() const { return out_2; }

  // noconvert
  const uint32_t* get_data() const {
    return (uint32_t*)data;
  }

private:
  logic<10> raddr1;
  logic<10> raddr2;
  logic<10> waddr;
  logic<32> wdata;
  logic<1>  wren;

  logic<32> data[1024];
  logic<32> out_1;
  logic<32> out_2;
};

//------------------------------------------------------------------------------
// verilator lint_on unusedsignal
