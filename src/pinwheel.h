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

struct BlockRam {
  void tick_read (logic<32> raddr, logic<1> rden);
  void tick_write(logic<32> waddr, logic<32> wdata, logic<4> wmask, logic<1> wren);
  uint32_t  data[16384];
  logic<32> out;
};

//------------------------------------------------------------------------------

struct BlockRegfile {
  void tick_read (logic<10> raddr1, logic<10> raddr2, logic<1> rden);
  void tick_write(logic<10> waddr, logic<32> wdata, logic<1> wren);
  uint32_t data[1024];
  logic<32> out_a;
  logic<32> out_b;
};

//------------------------------------------------------------------------------

struct Pinwheel {

  Pinwheel();
  Pinwheel* clone();
  size_t size_bytes() { return sizeof(*this); }

  static logic<32> unpack(logic<32> insn, logic<32> addr, logic<32> data);
  static logic<32> alu(logic<32> insn, logic<32> pc, logic<32> reg_a, logic<32> reg_b);
  static logic<32> pc_gen(logic<32> pc, logic<32> insn, logic<1> active, logic<32> reg_a, logic<32> reg_b);
  static logic<32> addr_gen(logic<32> insn, logic<32> reg_a);
  static logic<4>  mask_gen(logic<32> insn, logic<32> addr);

  void reset();
  void tock(logic<1> reset);
  void tick(logic<1> reset_in);

  void tick_code();
  void tick_data();
  void tick_regfile();

  static const int hart_count = 4;
  static const int vane_count = 4;

  uint64_t ticks;
  BlockRam code;
  BlockRam data;
  BlockRegfile regs;

  logic<5>  vane0_hart;
  logic<32> vane0_pc;
  logic<1>  vane0_enable;
  logic<1>  vane0_active;

  logic<5>  vane1_hart;
  logic<32> vane1_pc;
  logic<32> vane1_insn;
  logic<1>  vane1_enable;
  logic<1>  vane1_active;

  logic<5>  vane2_hart;
  logic<32> vane2_pc;
  logic<32> vane2_insn;
  logic<1>  vane2_enable;
  logic<1>  vane2_active;
  logic<32> vane2_mem_addr; // Copy of address, used to realign data after read
  logic<32> vane2_alu_out;  // Copy of alu output, used for register writeback

  logic<10> writeback_addr;
  logic<32> writeback_data;
  logic<1>  writeback_wren;

  logic<32> debug_reg;

  char console_buf[80*50];
  int console_x = 0;
  int console_y = 0;
};

//------------------------------------------------------------------------------
