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

enum RVOps {
  RV_LB,
  RV_LH,
  RV_LW,
  RV_LBU,
  RV_LHU,
  RV_SB,
  RV_SH,
  RV_SW,
  RV_SLL,
  RV_SLLI,

  RV_SRL,
  RV_SRLI,
  RV_SRA,
  RV_SRAI,
  RV_ADD,
  RV_ADDI,
  RV_SUB,
  RV_LUI,
  RV_AUIPC,
  RV_XOR,

  RV_XORI,
  RV_OR,
  RV_ORI,
  RV_AND,
  RV_ANDI,
  RV_SLT,
  RV_SLTI,
  RV_SLTU,
  RV_SLTUI,
  RV_BEQ,

  RV_BNE,
  RV_BLT,
  RV_BGE,
  RV_BLTU,
  RV_BGEU,
  RV_JAL,
  RV_JALR
};

struct DecodedOp {
  RVOps     op;
  logic<32> imm;
  logic<5>  ra;
  logic<5>  rb;
  logic<5>  rd;
};

//------------------------------------------------------------------------------

struct BlockRam {
  void tick_read (logic<32> raddr, logic<1> rden);
  void tick_write(logic<32> waddr, logic<32> wdata, logic<4> wmask, logic<1> wren);
  uint32_t  data[16384];
  logic<32> out;
};

//------------------------------------------------------------------------------

struct Regfile {
  void tick_read (logic<10> raddr1, logic<10> raddr2, logic<1> rden);
  void tick_write(logic<10> waddr, logic<32> wdata, logic<1> wren);
  uint32_t  data[1024];
  logic<32> out_a;
  logic<32> out_b;
};

//------------------------------------------------------------------------------

struct MemPort {
  logic<32> addr;
  logic<1>  rden;
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

  Pinwheel();
  Pinwheel* clone();
  size_t size_bytes() { return sizeof(*this); }

  static logic<32> unpack(logic<3> f3, logic<32> addr, logic<32> data);
  static logic<32> alu(logic<5> op, logic<3> f3, logic<7> f7, logic<32> imm, logic<32> pc, logic<32> reg_a, logic<32> reg_b);
  static logic<32> next_pc(logic<5> op, logic<3> f3, logic<32> imm, logic<32> pc, logic<32> reg_a, logic<32> reg_b);
  static logic<32> decode_imm(logic<32> insn);

  static MemPort mem_if(logic<5> op, logic<3> f3, logic<32> imm, logic<32> reg_a, logic<32> reg_b);
  static RegPortWrite writeback(logic<5> op, logic<5> rd, logic<32> rdata, logic<32> alu);

  void reset();
  void tock(logic<1> reset);
  void tick(logic<1> reset_in);

  void tick_code();
  void tick_data();
  logic<32> tick_bus(MemPort port);
  void tick_regfile();

  uint64_t ticks;

  logic<32> pc;
  logic<32> debug_reg;

  BlockRam code;
  BlockRam data;
  Regfile  regs;

  char console_buf[80*50];
  int console_x = 0;
  int console_y = 0;
};

//------------------------------------------------------------------------------
