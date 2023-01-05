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

  void reset();
  static logic<32> decode_imm(logic<32> insn);

  void tick_fetch  (logic<1> reset, logic<32> old_pc2, logic<32> old_insn1, logic<32> old_ra, logic<32> old_rb);
  void tick_write  (logic<1> reset);
  void tick_memory (logic<1> reset);
  void tick_execute(logic<1> reset);
  void tick_decode (logic<1> reset);

  logic<32> tock_memory();
  logic<32> tock_code();

  void tick_onecycle(logic<1> reset_in);
  void tick_twocycle(logic<1> reset_in);

  logic<32> pc1;
  logic<32> pc2;

  logic<32> insn1;
  logic<32> insn2;
  logic<32> bus_addr;
  logic<32> alu_out;
  logic<32> debug_reg;

  BlockRam code;
  BlockRam data;
  Regfile  regfile;

  char console_buf[80*50];
  int console_x = 0;
  int console_y = 0;

  uint64_t ticks;
};

//------------------------------------------------------------------------------
