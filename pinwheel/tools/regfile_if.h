#ifndef PINHWEEL_TOOLS_REGFILE_IF_H
#define PINHWEEL_TOOLS_REGFILE_IF_H

//------------------------------------------------------------------------------

#include "metron/metron_tools.h"

struct regfile_if {
  logic<13> raddr1;
  logic<13> raddr2;
  logic<13> waddr;
  logic<32> wdata;
  logic<1>  wren;
};

//------------------------------------------------------------------------------

#endif // PINHWEEL_TOOLS_REGFILE_IF_H
