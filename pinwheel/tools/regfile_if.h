#ifndef PINHWEEL_TOOLS_REGFILE_IF_H
#define PINHWEEL_TOOLS_REGFILE_IF_H

//------------------------------------------------------------------------------

#include "metron/metron_tools.h"

struct regfile_if {
  logic<8>  raddr1;
  logic<8>  raddr2;
  logic<8>  waddr;
  logic<32> wdata;
  logic<1>  wren;
};

//------------------------------------------------------------------------------

#endif // PINHWEEL_TOOLS_REGFILE_IF_H
