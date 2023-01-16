#pragma once
#include "metron_tools.h"

#include "block_ram.h"
// metron_noconvert
#include "console.h"
#include "constants.h"
#include "regfile.h"
#include "pinwheel_core.h"

//------------------------------------------------------------------------------

// verilator lint_off unusedsignal
// verilator lint_off varhidden

// FIXME remove this once everything hooked up
// verilator lint_off UNDRIVEN

class pinwheel {
public:

  pinwheel(const char* text_file = nullptr, const char* data_file = nullptr) {
    readmemh(text_file, code.data);
    readmemh(data_file, data_ram.data);
  }

  // metron_noconvert
  pinwheel* clone() {
    pinwheel* p = new pinwheel();
    memcpy(p, this, sizeof(*this));
    return p;
  }

  // metron_noconvert
  size_t size_bytes() {
    return sizeof(*this);
  }

  // metron_noconvert
  bool load_elf(const char* firmware_filename);

  //----------------------------------------
  // FIXME const local variable should not become parameter

  void tock(logic<1> reset_in) {

    logic<32> rs1_b  = b5(core.insn_b, 15) ? core.regs.get_rs1() : b32(0);
    logic<32> rs2_b  = b5(core.insn_b, 20) ? core.regs.get_rs2() : b32(0);
    logic<32> imm_b  = core.decode_imm(core.insn_b);
    logic<32> addr_b = b32(rs1_b + imm_b);

    //----------
    // Memory

    logic<1> code_cs_b     = b4(addr_b, 28) == 0x0;
    logic<1> console1_cs_b = b4(addr_b, 28) == 0x4;
    logic<1> console2_cs_b = b4(addr_b, 28) == 0x5;
    logic<1> console3_cs_b = b4(addr_b, 28) == 0x6;
    logic<1> console4_cs_b = b4(addr_b, 28) == 0x7;
    logic<1> data_cs_b     = b4(addr_b, 28) == 0x8;
    logic<1> regfile_cs_b  = b4(addr_b, 28) == 0xE;
    logic<1> debug_cs_b    = b4(addr_b, 28) == 0xF;

    logic<1> code_cs_c     = b4(core.addr_c, 28) == 0x0;
    logic<1> console1_cs_c = b4(core.addr_c, 28) == 0x4;
    logic<1> console2_cs_c = b4(core.addr_c, 28) == 0x5;
    logic<1> console3_cs_c = b4(core.addr_c, 28) == 0x6;
    logic<1> console4_cs_c = b4(core.addr_c, 28) == 0x7;
    logic<1> data_cs_c     = b4(core.addr_c, 28) == 0x8;
    logic<1> debug_cs_c    = b4(core.addr_c, 28) == 0xF;
    logic<1> regfile_cs_c  = b4(core.addr_c, 28) == 0xE;

    logic<32> data_out_c = 0;
    if (data_cs_c) {
      data_out_c = data_ram.rdata();
    }
    else if (debug_cs_c) {
      data_out_c = debug_reg;
    }
    else if (regfile_cs_c) {
      data_out_c = core.regs.get_rs1();
    }

    core.tock(reset_in, code.rdata(), data_out_c);

    //----------
    // Submod tocks

    code.tock    (b12(core.code_addr), core.code_wdata, core.code_wmask, core.code_wren);
    data_ram.tock(b12(core.bus_addr), core.bus_wdata, core.bus_wmask, core.bus_wren);

    // metron_noconvert
    console1.tock(console1_cs_b && b5(core.insn_b, 2) == RV32I::OP_STORE, core.bus_wdata);
    // metron_noconvert
    console2.tock(console2_cs_b && b5(core.insn_b, 2) == RV32I::OP_STORE, core.bus_wdata);
    // metron_noconvert
    console3.tock(console3_cs_b && b5(core.insn_b, 2) == RV32I::OP_STORE, core.bus_wdata);
    // metron_noconvert
    console4.tock(console4_cs_b && b5(core.insn_b, 2) == RV32I::OP_STORE, core.bus_wdata);

    //----------
    // Signal writeback

    next_debug_reg = (b5(core.insn_b, 2) == RV32I::OP_STORE) && debug_cs_b ? rs2_b : debug_reg;
  }

  //----------------------------------------

  logic<32> get_debug() const {
    return debug_reg;
  }

  //----------------------------------------
  // FIXME trace modules individually

  void tick(logic<1> reset_in) {
    core.tick(reset_in);

    if (reset_in) {
      debug_reg = 0;
      // metron_noconvert
      ticks     = 0;
    }
    else {
      debug_reg = next_debug_reg;
      // metron_noconvert
      ticks     = ticks + 1;
    }

    code.tick();
    data_ram.tick();
    core.regs.tick();

    // metron_noconvert
    console1.tick(reset_in);
    // metron_noconvert
    console2.tick(reset_in);
    // metron_noconvert
    console3.tick(reset_in);
    // metron_noconvert
    console4.tick(reset_in);
  }

  // metron_noconvert
  uint32_t* get_code() { return code.get_data(); }
  // metron_noconvert
  uint32_t* get_data() { return data_ram.get_data(); }

  //----------------------------------------

  // metron_internal
  pinwheel_core core;
  logic<32> next_debug_reg;
  logic<32> debug_reg;
  logic<32> gpio_dir;
  logic<32> gpio_in;
  logic<32> gpio_out;
  block_ram  code;
  block_ram  data_ram; // FIXME having this named data and a field inside block_ram named data breaks context resolve

  // metron_noconvert
  Console console1;
  // metron_noconvert
  Console console2;
  // metron_noconvert
  Console console3;
  // metron_noconvert
  Console console4;
  // metron_noconvert
  uint64_t ticks;
};

// verilator lint_on unusedsignal

//------------------------------------------------------------------------------
