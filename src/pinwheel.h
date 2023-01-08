#pragma once
#include "metron_tools.h"
#include "constants.h"

//------------------------------------------------------------------------------

struct BlockRam {
  void tock(logic<32> addr, logic<32> wdata, logic<4> wmask, logic<1> wren);
  void tick();

  logic<32> addr;
  logic<32> wdata;
  logic<4>  wmask;
  logic<1>  wren;

  uint32_t  data[16384];
  logic<32> out;
};

//------------------------------------------------------------------------------

struct Regfile {
  void tock(logic<10> raddr1, logic<10> raddr2, logic<10> waddr, logic<32> wdata, logic<1> wren);
  void tick();

  logic<10> raddr1;
  logic<10> raddr2;
  logic<10> waddr;
  logic<32> wdata;
  logic<1>  wren;

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

struct Console {
  void tock(logic<1> wrcs, logic<32> reg_b);
  void tick(logic<1> reset);

  static const int width =64;
  static const int height=16;

  char buf[width*height];
  int  x = 0;
  int  y = 0;
  logic<1>  wrcs;
  logic<32> reg_b;
};

//------------------------------------------------------------------------------

struct Pinwheel {

  Pinwheel() {}
  Pinwheel* clone();
  size_t size_bytes() { return sizeof(*this); }

  void reset_mem();
  static logic<32> decode_imm(logic<32> insn);

  logic<32> execute_alu   (logic<32> insn, logic<32> reg_a, logic<32> reg_b) const;
  logic<32> execute_system(logic<32> insn) const;

  void tick_twocycle(logic<1> reset_in) const;
  void tock_twocycle(logic<1> reset_in) const;

  //----------

  logic<5>  next_hart_a;
  logic<32> next_pc_a;

  logic<32> next_insn_b;

  logic<32> next_addr_c;
  logic<32> next_result_c;

  logic<10> next_wb_addr_d;
  logic<32> next_wb_data_d;
  logic<1>  next_wb_wren_d;

  logic<32> next_debug_reg;

  //----------

  logic<5>  hart_a;
  logic<32> pc_a;

  logic<5>  hart_b;
  logic<32> pc_b;
  logic<32> insn_b;

  logic<5>  hart_c;
  logic<32> pc_c;
  logic<32> insn_c;
  logic<32> addr_c;
  logic<32> result_c;

  logic<5>  hart_d;
  logic<32> pc_d;
  logic<32> insn_d;
  logic<32> result_d;
  logic<10> wb_addr_d;
  logic<32> wb_data_d;
  logic<1>  wb_wren_d;

  logic<32> debug_reg;

  logic<32> gpio_dir;
  logic<32> gpio_in;
  logic<32> gpio_out;

  BlockRam  code;
  BlockRam  data;
  Regfile   regfile;

  Console console1;
  Console console2;
  Console console3;
  Console console4;

  /*
  char console_buf[80*50];
  int console_x = 0;
  int console_y = 0;
  logic<1>  console_wrcs;
  logic<32> console_reg_b;
  */

  uint64_t ticks;
};

//------------------------------------------------------------------------------
