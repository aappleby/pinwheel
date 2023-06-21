`ifndef CONSTANTS_H
`define CONSTANTS_H

`include "metron_tools.sv"

// verilator lint_off unusedparam
package RV32I;
  parameter int OP_LOAD    = 5'b00000;
  parameter int OP_LOADFP  = 5'b00001;
  parameter int OP_CUSTOM0 = 5'b00010;
  parameter int OP_MISCMEM = 5'b00011;
  parameter int OP_OPIMM   = 5'b00100;
  parameter int OP_AUIPC   = 5'b00101;
  parameter int OP_OPIMM32 = 5'b00110;
  parameter int OP_48B1    = 5'b00111;

  parameter int OP_STORE   = 5'b01000;
  parameter int OP_STOREFP = 5'b01001;
  parameter int OP_CUSTOM1 = 5'b01010;
  parameter int OP_AMO     = 5'b01011;
  parameter int OP_OP      = 5'b01100;
  parameter int OP_LUI     = 5'b01101;
  parameter int OP_OP32    = 5'b01110;
  parameter int OP_64B     = 5'b01111;

  parameter int OP_MADD    = 5'b10000;
  parameter int OP_MSUB    = 5'b10001;
  parameter int OP_NMSUB   = 5'b10010;
  parameter int OP_NMADD   = 5'b10011;
  parameter int OP_OPFP    = 5'b10100;
  parameter int OP_RES1    = 5'b10101;
  parameter int OP_CUSTOM2 = 5'b10110;
  parameter int OP_48B2    = 5'b10111;

  parameter int OP_BRANCH  = 5'b11000;
  parameter int OP_JALR    = 5'b11001;
  parameter int OP_RES2    = 5'b11010;
  parameter int OP_JAL     = 5'b11011;
  parameter int OP_SYSTEM  = 5'b11100;
  parameter int OP_RES3    = 5'b11101;
  parameter int OP_CUSTOM3 = 5'b11110;
  parameter int OP_80B     = 5'b11111;

  parameter int F3_BEQ     = 3'b000;
  parameter int F3_BNE     = 3'b001;
  parameter int F3_BLT     = 3'b100;
  parameter int F3_BGE     = 3'b101;
  parameter int F3_BLTU    = 3'b110;
  parameter int F3_BGEU    = 3'b111;

  parameter int F3_LB      = 3'b000;
  parameter int F3_LH      = 3'b001;
  parameter int F3_LW      = 3'b010;
  parameter int F3_LBU     = 3'b100;
  parameter int F3_LHU     = 3'b101;

  parameter int F3_SB      = 3'b000;
  parameter int F3_SH      = 3'b001;
  parameter int F3_SW      = 3'b010;

  parameter int F3_ADDI = 3'b000;
  parameter int F3_SLI     = 3'b001;
  parameter int F3_SLTI    = 3'b010;
  parameter int F3_SLTIU   = 3'b011;
  parameter int F3_XORI    = 3'b100;
  parameter int F3_SRI     = 3'b101;
  parameter int F3_ORI     = 3'b110;
  parameter int F3_ANDI    = 3'b111;

  parameter int F3_ADDSUB  = 3'b000;
  parameter int F3_SL      = 3'b001;
  parameter int F3_SLT     = 3'b010;
  parameter int F3_SLTU    = 3'b011;
  parameter int F3_XOR     = 3'b100;
  parameter int F3_SR      = 3'b101;
  parameter int F3_OR      = 3'b110;
  parameter int F3_AND     = 3'b111;

  parameter int F3_CSRRW   = 3'b001;
  parameter int F3_CSRRS   = 3'b010;
  parameter int F3_CSRRC   = 3'b011;
  parameter int F3_CSRRWI  = 3'b101;
  parameter int F3_CSRRSI  = 3'b110;
  parameter int F3_CSRRCI  = 3'b111;
endpackage
// verilator lint_on unusedparam

`endif
