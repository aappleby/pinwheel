#ifndef TILELINK_H
#define TILELINK_H

#include "metron/metron_tools.h"

//------------------------------------------------------------------------------

// verilator lint_off unusedparam
namespace TL {
  const int PutFullData = 0;
  const int PutPartialData = 1;
  const int ArithmeticData = 2;
  const int LogicalData = 3;
  const int Get = 4;
  const int Intent = 5;
  const int Acquire = 6;

  const int AccessAck = 0;
  const int AccessAckData = 1;
  const int HintAck = 2;
  const int Grant = 4;
  const int GrantData = 5;
  const int ReleaseAck = 6;

  /*
  // FIXME metron doesn't like functions in namespaces
  logic<32> expand_bitmask(logic<4> mask) {
    return cat(dup<8>(mask[3]), dup<8>(mask[2]), dup<8>(mask[1]), dup<8>(mask[0]));
  }
  */
};
// verilator lint_on unusedparam

//------------------------------------------------------------------------------

struct tilelink_a {
  // FIXME enums inside structs are broken in Metron
  logic<3>  a_opcode;
  logic<3>  a_param;
  logic<3>  a_size;
  logic<1>  a_source;
  logic<32> a_address;
  logic<4>  a_mask;
  logic<32> a_data;
  logic<1>  a_valid;
  logic<1>  a_ready;
};

//------------------------------------------------------------------------------

struct tilelink_d {
  logic<3>  d_opcode;
  logic<2>  d_param;
  logic<3>  d_size;
  logic<1>  d_source;
  logic<3>  d_sink;
  logic<32> d_data;
  logic<1>  d_error;
  logic<1>  d_valid;
  logic<1>  d_ready;
};

//------------------------------------------------------------------------------

#endif
