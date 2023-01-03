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
  void tick_read (logic<32> raddr);
  void tick_write(logic<32> waddr, logic<32> wdata, logic<4> wmask, logic<1> wren);
  uint32_t  data[16384];
  logic<32> out;
};

//------------------------------------------------------------------------------

struct Regfile {
  void tick_read (logic<10> raddr1, logic<10> raddr2);
  void tick_write(logic<10> waddr, logic<32> wdata, logic<1> wren);
  uint32_t  data[1024];
  logic<32> out_a;
  logic<32> out_b;
};

//------------------------------------------------------------------------------

struct MemPort {
  logic<32> addr;
  logic<32> wdata;
  logic<4>  wmask;
  logic<1>  wren;
};

struct RegPortWrite {
  logic<10> addr;
  logic<32> wdata;
  logic<1>  wren;
};

//------------------------------------------------------------------------------

struct Pinwheel {

  Pinwheel() {}
  Pinwheel* clone();
  size_t size_bytes() { return sizeof(*this); }

  static logic<32> tock_unpack(logic<3> f3, logic<32> addr, logic<32> data);
  static logic<32> tock_alu(logic<5> op, logic<3> f3, logic<7> f7, logic<32> imm, logic<32> pc, logic<32> reg_a, logic<32> reg_b);
  static logic<32> tock_pc(logic<5> op, logic<3> f3, logic<32> imm, logic<32> pc, logic<32> reg_a, logic<32> reg_b);
  static logic<32> tock_imm(logic<32> insn);

  static MemPort tock_bus(logic<5> op, logic<3> f3, logic<32> imm, logic<32> reg_a, logic<32> reg_b);
  static RegPortWrite tock_wb(logic<5> op, logic<5> rd, logic<32> rdata, logic<32> alu);

  void reset();
  void tick_singlecycle(logic<1> reset_in);

  logic<32> tick_bus(MemPort port);

  uint64_t ticks;

  logic<32> pc;
  logic<32> debug_reg;

  BlockRam code;
  BlockRam data;
  Regfile  regfile;

  char console_buf[80*50];
  int console_x = 0;
  int console_y = 0;
};

//------------------------------------------------------------------------------
