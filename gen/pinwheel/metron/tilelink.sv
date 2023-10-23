`ifndef PINWHEEL_RTL_TILELINK_H
`define PINWHEEL_RTL_TILELINK_H

`include "metron/metron_tools.sv"

//------------------------------------------------------------------------------
// verilator lint_off unusedparam

package TL;
  parameter /*static*/ /*const*/ int PutFullData = 0;
  parameter /*static*/ /*const*/ int PutPartialData = 1;
  parameter /*static*/ /*const*/ int ArithmeticData = 2;
  parameter /*static*/ /*const*/ int LogicalData = 3;
  parameter /*static*/ /*const*/ int Get = 4;
  parameter /*static*/ /*const*/ int Intent = 5;
  parameter /*static*/ /*const*/ int Acquire = 6;

  parameter /*static*/ /*const*/ int AccessAck = 0;
  parameter /*static*/ /*const*/ int AccessAckData = 1;
  parameter /*static*/ /*const*/ int HintAck = 2;
  parameter /*static*/ /*const*/ int Grant = 4;
  parameter /*static*/ /*const*/ int GrantData = 5;
  parameter /*static*/ /*const*/ int ReleaseAck = 6;

  /*
  // FIXME metron doesn't like functions in namespaces
  logic<32> expand_bitmask(logic<4> mask) {
    return cat(dup<8>(mask[3]), dup<8>(mask[2]), dup<8>(mask[1]), dup<8>(mask[0]));
  }
  */
endpackage

//----------------------------------------

typedef struct packed {
  // FIXME enums inside structs are broken in Metron
  logic[2:0]  a_opcode;
  logic[2:0]  a_param;
  logic[2:0]  a_size;
  logic  a_source;
  logic[31:0] a_address;
  logic[3:0]  a_mask;
  logic[31:0] a_data;
  logic  a_valid;
  logic  a_ready;
} tilelink_a;

//----------------------------------------

typedef struct packed {
  logic[2:0]  d_opcode;
  logic[1:0]  d_param;
  logic[2:0]  d_size;
  logic  d_source;
  logic[2:0]  d_sink;
  logic[31:0] d_data;
  logic  d_error;
  logic  d_valid;
  logic  d_ready;
} tilelink_d;

// verilator lint_on unusedparam
//------------------------------------------------------------------------------

`endif // PINWHEEL_RTL_TILELINK_H
