#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "metron/metron_tools.h"

// verilator lint_off unusedparam
namespace RV32I {
  static const int OP_LOAD    = 0b0000011;
  static const int OP_LOADFP  = 0b0000111;
  static const int OP_CUSTOM0 = 0b0001011;
  static const int OP_MISCMEM = 0b0001111;
  static const int OP_OPIMM   = 0b0010011;
  static const int OP_AUIPC   = 0b0010111;
  static const int OP_OPIMM32 = 0b0011011;
  static const int OP_48B1    = 0b0011111;

  static const int OP_STORE   = 0b0100011;
  static const int OP_STOREFP = 0b0100111;
  static const int OP_CUSTOM1 = 0b0101011;
  static const int OP_AMO     = 0b0101111;
  static const int OP_OP      = 0b0110011;
  static const int OP_LUI     = 0b0110111;
  static const int OP_OP32    = 0b0111011;
  static const int OP_64B     = 0b0111111;

  static const int OP_MADD    = 0b1000011;
  static const int OP_MSUB    = 0b1000111;
  static const int OP_NMSUB   = 0b1001011;
  static const int OP_NMADD   = 0b1001111;
  static const int OP_OPFP    = 0b1010011;
  static const int OP_RES1    = 0b1010111;
  static const int OP_CUSTOM2 = 0b1011011;
  static const int OP_48B2    = 0b1011111;

  static const int OP_BRANCH  = 0b1100011;
  static const int OP_JALR    = 0b1100111;
  static const int OP_RES2    = 0b1101011;
  static const int OP_JAL     = 0b1101111;
  static const int OP_SYSTEM  = 0b1110011;
  static const int OP_RES3    = 0b1110111;
  static const int OP_CUSTOM3 = 0b1111011;
  static const int OP_80B     = 0b1111111;

  static const int F3_BEQ     = 0b000;
  static const int F3_BNE     = 0b001;
  static const int F3_BEQU    = 0b010;
  static const int F3_BNEU    = 0b011;
  static const int F3_BLT     = 0b100;
  static const int F3_BGE     = 0b101;
  static const int F3_BLTU    = 0b110;
  static const int F3_BGEU    = 0b111;

  static const int F3_LB      = 0b000;
  static const int F3_LH      = 0b001;
  static const int F3_LW      = 0b010;
  static const int F3_LD      = 0b011;
  static const int F3_LBU     = 0b100;
  static const int F3_LHU     = 0b101;
  static const int F3_LWU     = 0b110;
  static const int F3_LDU     = 0b111;

  static const int F3_SB      = 0b000;
  static const int F3_SH      = 0b001;
  static const int F3_SW      = 0b010;

  static const int F3_ADDSUB  = 0b000;
  static const int F3_SL      = 0b001;
  static const int F3_SLT     = 0b010;
  static const int F3_SLTU    = 0b011;
  static const int F3_XOR     = 0b100;
  static const int F3_SR      = 0b101;
  static const int F3_OR      = 0b110;
  static const int F3_AND     = 0b111;

  static const int F3_CSRRW   = 0b001;
  static const int F3_CSRRS   = 0b010;
  static const int F3_CSRRC   = 0b011;
  static const int F3_CSRRWI  = 0b101;
  static const int F3_CSRRSI  = 0b110;
  static const int F3_CSRRCI  = 0b111;

  static const int MVENDORID = 0xF11;
  static const int MARCHID   = 0xF12;
  static const int MIMPID    = 0xF13;
  static const int MHARTID   = 0xF14;
};

struct rv32_rtype {
  uint32_t op  : 7;
  uint32_t rd  : 5;
  uint32_t f3  : 3;
  uint32_t rs1 : 5;
  uint32_t rs2 : 5;
  uint32_t f7  : 7;
};

struct rv32_itype {
  uint32_t op : 7;
  uint32_t rd : 5;
  uint32_t f3 : 3;
  uint32_t rs1 : 5;
  uint32_t imm_11_0 : 12;
};


struct rv32_stype {
  uint32_t op : 7;
  uint32_t imm_4_0 : 5;
  uint32_t f3 : 3;
  uint32_t rs1 : 5;
  uint32_t rs2 : 5;
  uint32_t imm_11_5 : 7;
};

struct rv32_btype {
  uint32_t op : 7;
  uint32_t imm_11 : 1;
  uint32_t imm_4_1 : 4;
  uint32_t f3 : 3;
  uint32_t rs1 : 5;
  uint32_t rs2 : 5;
  uint32_t imm_10_5 : 6;
  uint32_t imm_12 : 1;
};

struct rv32_utype {
  uint32_t op : 7;
  uint32_t rd : 5;
  uint32_t imm_31_12 : 20;
};

struct rv32_jtype {
  uint32_t op : 7;
  uint32_t rd : 5;
  uint32_t imm_19_12 : 8;
  uint32_t imm_11 : 1;
  uint32_t imm_10_1 : 10;
  uint32_t imm_20: 1;
};

struct rv32_ctype {
  uint32_t op  : 7;
  uint32_t rd  : 5;
  uint32_t f3  : 3;
  uint32_t rs1 : 5;
  uint32_t csr : 12;
};

union rv32_insn {
  logic<32>  raw;
  rv32_rtype r;
  rv32_itype i;
  rv32_stype s;
  rv32_btype b;
  rv32_utype u;
  rv32_jtype j;
  rv32_ctype c;
};

//static_assert(sizeof(rv32_insn) == 4);

// verilator lint_on unusedparam
#endif
