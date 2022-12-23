#pragma once
#include "metron_tools.h"
#include "constants.h"

static const int OP_LOAD    = 0b00000;
static const int OP_ALUI    = 0b00100;
static const int OP_AUIPC   = 0b00101;
static const int OP_STORE   = 0b01000;
static const int OP_ALU     = 0b01100;
static const int OP_LUI     = 0b01101;
static const int OP_BRANCH  = 0b11000;
static const int OP_JALR    = 0b11001;
static const int OP_JAL     = 0b11011;
static const int OP_SYS     = 0b11100;

//------------------------------------------------------------------------------

struct Vane {
  logic<5>  hart;
  logic<32> pc;
  logic<32> insn;
  logic<1>  enable;
  logic<1>  active;
};

//------------------------------------------------------------------------------

struct BlockRam {
  void tick(logic<32> addr, logic<32> wdata, logic<4> wmask);
  uint32_t  data[16384];
  logic<32> out;
};

//------------------------------------------------------------------------------

struct BlockRegfile {
  void tick(logic<10> raddr1, logic<10> raddr2, logic<10> waddr, logic<1> wren, logic<32> wdata);
  uint32_t data[1024];
  logic<32> out_a;
  logic<32> out_b;
};

//------------------------------------------------------------------------------

struct Pinwheel {
  static logic<32> unpack(logic<32> insn, logic<32> addr, logic<32> data);
  static logic<32> alu(logic<32> insn, logic<32> pc, logic<32> reg_a, logic<32> reg_b);
  static logic<1>  take_branch(logic<32> insn, logic<32> reg_a, logic<32> reg_b);
  static logic<32> pc_gen(logic<32> pc, logic<32> insn, logic<1> active, logic<1> take_branch, logic<32> reg_a);
  static logic<32> addr_gen(logic<32> insn, logic<32> reg_a);
  static logic<4>  mask_gen(logic<32> insn, logic<32> addr);

  void reset();
  void tock(logic<1> reset);
  void tick(logic<1> reset_in);

  static const int hart_count = 4;
  static const int vane_count = 4;

  BlockRam code;
  BlockRam data;
  BlockRegfile regs;

  logic<32> temp_addr; // Copy of address, used to realign data after read
  logic<32> temp_alu;  // Copy of alu output, used for register writeback

  Vane vane0;
  Vane vane1;
  Vane vane2;
};

//------------------------------------------------------------------------------
