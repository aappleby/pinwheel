#pragma once
#include "metron_tools.h"

//------------------------------------------------------------------------------

class Regfile {
public:

  void tock(logic<10> raddr1_ , logic<10> raddr2_, logic<10> waddr_, logic<32> wdata_, logic<1> wren_) {
    raddr1 = raddr1_;
    raddr2 = raddr2_;
    waddr  = waddr_;
    wdata  = wdata_;
    wren   = wren_;
  }

  logic<32> get_rs1() const {
    return data[raddr1];
  }

  logic<32> get_rs2() const {
    return data[raddr2];
  }

  void tick() {
    if (wren) data[waddr] = wdata;
  }

  // noconvert
  const uint32_t* get_data() const {
    return data;
  }

private:
  logic<10> raddr1;
  logic<10> raddr2;
  logic<10> waddr;
  logic<32> wdata;
  logic<1>  wren;

  uint32_t  data[1024];
};

//------------------------------------------------------------------------------
