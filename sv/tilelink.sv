`include "metron_tools.sv"

typedef struct packed {
  logic[2:0]  a_opcode;
  logic[2:0]  a_param;
  logic[2:0]  a_size;
  logic  a_source;
  logic[31:0] a_address;
  logic[3:0]  a_mask;
  logic[31:0] a_data;
  logic  a_valid;
  logic  a_ready;
} TileLinkA;

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
} TileLinkD;
