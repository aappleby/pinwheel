#pragma once
#include "metron_tools.h"

//------------------------------------------------------------------------------

struct BlockRam {
  void tock(logic<32> addr, logic<32> wdata, logic<4> wmask, logic<1> wren);
  void tick();

  logic<32> addr;
  logic<32> wdata;
  logic<4>  wmask;
  logic<1>  wren;

  uint32_t  data[16384];
  logic<32> out;
};

//------------------------------------------------------------------------------
