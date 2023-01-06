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
static const int OP_SYSTEM  = 0b11100;

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
  logic<32> out_rs1;
  logic<32> out_rs2;
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

  void tick_console(logic<32> reg_b);

  logic<32> get_memory();

  logic<32> execute_alu   (logic<32> insn, logic<32> reg_a, logic<32> reg_b) const;
  logic<32> execute_custom(logic<32> insn, logic<32> reg_a);
  logic<32> execute_system(logic<32> insn) const;

  void tick_twocycle(logic<1> reset_in) const;

  logic<5>  hart_a;
  logic<32> pc_a;
  logic<32> insn_a;
  logic<32> result_a;

  logic<5>  hart_b;
  logic<32> pc_b;
  logic<32> insn_b;

  logic<10> writeback_addr;
  logic<32> writeback_data;
  logic<1>  writeback_wren;

  logic<32> debug_reg;
  //logic<1>  force_jump;
  //logic<32> jump_dest;

  BlockRam  code;
  BlockRam  data;
  Regfile   regfile;

  char console_buf[80*50];
  int console_x = 0;
  int console_y = 0;

  uint64_t ticks;
};

//------------------------------------------------------------------------------
