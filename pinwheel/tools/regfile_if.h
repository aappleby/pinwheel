#ifndef PINHWEEL_TOOLS_REGFILE_IF_H
#define PINHWEEL_TOOLS_REGFILE_IF_H

//------------------------------------------------------------------------------

#include "metron/metron_tools.h"

// max of 128 regfiles w/ 32 regs each

struct regfile_if {
  logic<12> raddr1;
  logic<12> raddr2;
  logic<12> waddr;
  logic<32> wdata;
  logic<1>  wren;
};

//------------------------------------------------------------------------------

#endif // PINHWEEL_TOOLS_REGFILE_IF_H
