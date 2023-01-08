#pragma once
#include "metron_tools.h"

//------------------------------------------------------------------------------

struct Regfile {
  void tock(logic<10> raddr1, logic<10> raddr2, logic<10> waddr, logic<32> wdata, logic<1> wren);
  void tick();

  logic<10> raddr1;
  logic<10> raddr2;
  logic<10> waddr;
  logic<32> wdata;
  logic<1>  wren;

  uint32_t  data[1024];
  logic<32> out_rs1;
  logic<32> out_rs2;
};

//------------------------------------------------------------------------------
