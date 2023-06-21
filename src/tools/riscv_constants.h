#ifndef CONSTANTS_H
#define CONSTANTS_H

#pragma once
#include "metron_tools.h"

// verilator lint_off unusedparam
namespace RV32I {
  const int OP_LOAD    = 0b00000;
  const int OP_LOADFP  = 0b00001;
  const int OP_CUSTOM0 = 0b00010;
  const int OP_MISCMEM = 0b00011;
  const int OP_OPIMM   = 0b00100;
  const int OP_AUIPC   = 0b00101;
  const int OP_OPIMM32 = 0b00110;
  const int OP_48B1    = 0b00111;

  const int OP_STORE   = 0b01000;
  const int OP_STOREFP = 0b01001;
  const int OP_CUSTOM1 = 0b01010;
  const int OP_AMO     = 0b01011;
  const int OP_OP      = 0b01100;
  const int OP_LUI     = 0b01101;
  const int OP_OP32    = 0b01110;
  const int OP_64B     = 0b01111;

  const int OP_MADD    = 0b10000;
  const int OP_MSUB    = 0b10001;
  const int OP_NMSUB   = 0b10010;
  const int OP_NMADD   = 0b10011;
  const int OP_OPFP    = 0b10100;
  const int OP_RES1    = 0b10101;
  const int OP_CUSTOM2 = 0b10110;
  const int OP_48B2    = 0b10111;

  const int OP_BRANCH  = 0b11000;
  const int OP_JALR    = 0b11001;
  const int OP_RES2    = 0b11010;
  const int OP_JAL     = 0b11011;
  const int OP_SYSTEM  = 0b11100;
  const int OP_RES3    = 0b11101;
  const int OP_CUSTOM3 = 0b11110;
  const int OP_80B     = 0b11111;

  const int F3_BEQ     = 0b000;
  const int F3_BNE     = 0b001;
  const int F3_BLT     = 0b100;
  const int F3_BGE     = 0b101;
  const int F3_BLTU    = 0b110;
  const int F3_BGEU    = 0b111;

  const int F3_LB      = 0b000;
  const int F3_LH      = 0b001;
  const int F3_LW      = 0b010;
  const int F3_LBU     = 0b100;
  const int F3_LHU     = 0b101;

  const int F3_SB      = 0b000;
  const int F3_SH      = 0b001;
  const int F3_SW      = 0b010;

  const int F3_ADDI = 0b000;
  const int F3_SLI     = 0b001;
  const int F3_SLTI    = 0b010;
  const int F3_SLTIU   = 0b011;
  const int F3_XORI    = 0b100;
  const int F3_SRI     = 0b101;
  const int F3_ORI     = 0b110;
  const int F3_ANDI    = 0b111;

  const int F3_ADDSUB  = 0b000;
  const int F3_SL      = 0b001;
  const int F3_SLT     = 0b010;
  const int F3_SLTU    = 0b011;
  const int F3_XOR     = 0b100;
  const int F3_SR      = 0b101;
  const int F3_OR      = 0b110;
  const int F3_AND     = 0b111;

  const int F3_CSRRW   = 0b001;
  const int F3_CSRRS   = 0b010;
  const int F3_CSRRC   = 0b011;
  const int F3_CSRRWI  = 0b101;
  const int F3_CSRRSI  = 0b110;
  const int F3_CSRRCI  = 0b111;
}
// verilator lint_on unusedparam

#endif
