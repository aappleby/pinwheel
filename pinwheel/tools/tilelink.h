#ifndef PINWHEEL_RTL_TILELINK_H
#define PINWHEEL_RTL_TILELINK_H

#include "metron/metron_tools.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedparam

namespace TL {
  static const int PutFullData = 0;
  static const int PutPartialData = 1;
  static const int ArithmeticData = 2;
  static const int LogicalData = 3;
  static const int Get = 4;
  static const int Intent = 5;
  static const int Acquire = 6;

  static const int AccessAck = 0;
  static const int AccessAckData = 1;
  static const int HintAck = 2;
  static const int Grant = 4;
  static const int GrantData = 5;
  static const int ReleaseAck = 6;
};

//----------------------------------------

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

//----------------------------------------

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

// verilator lint_on unusedparam
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_TILELINK_H
