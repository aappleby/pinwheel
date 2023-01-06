#pragma once
#include "metron_tools.h"
#include "constants.h"

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
  logic<5>  hart_c;
  logic<5>  hart_d;
  logic<32> pc_a;

  logic<5>  hart_b;
  logic<32> pc_b;
  logic<32> insn_b;

  logic<32> insn_c;
  logic<32> result_c;

  logic<10> wb_addr_d;
  logic<32> wb_data_d;
  logic<1>  wb_wren_d;

  logic<32> debug_reg;

  BlockRam  code;
  BlockRam  data;
  Regfile   regfile;

  char console_buf[80*50];
  int console_x = 0;
  int console_y = 0;

  uint64_t ticks;
};

//------------------------------------------------------------------------------
