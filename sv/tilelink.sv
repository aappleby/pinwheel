`ifndef TILELINK_H
`define TILELINK_H

`include "metron_tools.sv"

//------------------------------------------------------------------------------

// verilator lint_off unusedparam
package TL;
  parameter int PutFullData = 0;
  parameter int PutPartialData = 1;
  parameter int ArithmeticData = 2;
  parameter int LogicalData = 3;
  parameter int Get = 4;
  parameter int Intent = 5;
  parameter int Acquire = 6;

  parameter int AccessAck = 0;
  parameter int AccessAckData = 1;
  parameter int HintAck = 2;
  parameter int Grant = 4;
  parameter int GrantData = 5;
  parameter int ReleaseAck = 6;

  /*
  // FIXME metron doesn't like functions in namespaces
  logic<32> expand_bitmask(logic<4> mask) {
    return cat(dup<8>(mask[3]), dup<8>(mask[2]), dup<8>(mask[1]), dup<8>(mask[0]));
  }
  */
endpackage
// verilator lint_on unusedparam

//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

`endif
