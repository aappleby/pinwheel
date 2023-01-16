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

    logic<5> op_b   = b5(core.insn_b, 2);
    logic<5> rda_b  = b5(core.insn_b, 7);
    logic<3> f3_b   = b3(core.insn_b, 12);
    logic<5> rs1a_b = b5(core.insn_b, 15);
    logic<5> rs2a_b = b5(core.insn_b, 20);
    logic<7> f7_b   = b7(core.insn_b, 25);

    logic<5> op_c   = b5(core.insn_c, 2);
    logic<5> rd_c   = b5(core.insn_c, 7);
    logic<3> f3_c   = b3(core.insn_c, 12);

    logic<32> rs1_b  = rs1a_b ? core.regs.get_rs1() : b32(0);
    logic<32> rs2_b  = rs2a_b ? core.regs.get_rs2() : b32(0);
    logic<32> imm_b  = core.decode_imm(core.insn_b);
    logic<32> addr_b = b32(rs1_b + imm_b);

    logic<32> temp_pc_a = 0;

    core.tock(reset_in, code.rdata());

    //----------
    // Fetch

    {
      if (core.pc_b) {
        logic<1> eq  = rs1_b == rs2_b;
        logic<1> slt = signed(rs1_b) < signed(rs2_b);
        logic<1> ult = rs1_b < rs2_b;

        logic<1> take_branch;
        switch (f3_b) {
          case 0:  take_branch =   eq; break;
          case 1:  take_branch =  !eq; break;
          case 2:  take_branch =   eq; break;
          case 3:  take_branch =  !eq; break;
          case 4:  take_branch =  slt; break;
          case 5:  take_branch = !slt; break;
          case 6:  take_branch =  ult; break;
          case 7:  take_branch = !ult; break;
          default: take_branch =    0; break;
        }

        switch (op_b) {
          case RV32I::OP_BRANCH:  temp_pc_a = take_branch ? core.pc_b + imm_b : core.pc_b + b32(4); break;
          case RV32I::OP_JAL:     temp_pc_a = core.pc_b + imm_b; break;
          case RV32I::OP_JALR:    temp_pc_a = addr_b; break;
          default:                temp_pc_a = core.pc_b + 4; break;
        }
      }
    }

    //----------
    // Decode

    logic<32> insn_a = code.rdata();
    logic<5> rs1a_a  = b5(insn_a, 15);
    logic<5> rs2a_a  = b5(insn_a, 20);

    core.next_insn_b = core.pc_a == 0 ? b32(0) : insn_a;
    core.next_addr_c = addr_b;

    //----------
    // Execute

    switch(op_b) {
      case RV32I::OP_JAL:     core.next_result_c = core.pc_b + 4;     break;
      case RV32I::OP_JALR:    core.next_result_c = core.pc_b + 4;     break;
      case RV32I::OP_LUI:     core.next_result_c = imm_b;        break;
      case RV32I::OP_AUIPC:   core.next_result_c = core.pc_b + imm_b; break;
      case RV32I::OP_LOAD:    core.next_result_c = addr_b;       break;
      case RV32I::OP_STORE:   core.next_result_c = rs2_b;        break;
      case RV32I::OP_CUSTOM0: {
        core.next_result_c = 0;
        if (f3_b == 0) {
          // Switch the other thread to another hart
          core.next_addr_c   = rs1_b;
          core.next_result_c = rs2_b;
        }
        else if (f3_b == 1) {
          // Yield to another hart
          core.next_result_c  = temp_pc_a;
          core.next_hart_a    = rs1_b;
          temp_pc_a      = rs2_b;
        }
        break;
      }
      case RV32I::OP_SYSTEM:  core.next_result_c = core.execute_system(core.insn_b); break;
      default:                core.next_result_c = core.execute_alu   (core.insn_b, rs1_b, rs2_b); break;
    }

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

    logic<4>       temp_mask_b = 0;
    if (f3_b == 0) temp_mask_b = 0b0001;
    if (f3_b == 1) temp_mask_b = 0b0011;
    if (f3_b == 2) temp_mask_b = 0b1111;
    if (addr_b[0]) temp_mask_b = temp_mask_b << 1;
    if (addr_b[1]) temp_mask_b = temp_mask_b << 2;

    logic<4>       temp_mask_c = 0;
    if (f3_c == 0) temp_mask_c = 0b0001;
    if (f3_c == 1) temp_mask_c = 0b0011;
    if (f3_c == 2) temp_mask_c = 0b1111;
    if (core.addr_c[0]) temp_mask_c = temp_mask_c << 1;
    if (core.addr_c[1]) temp_mask_c = temp_mask_c << 2;

    //----------
    // Write

    logic<1> code_cs_c     = b4(core.addr_c, 28) == 0x0 && temp_pc_a == 0;
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

    logic<32>        unpacked_c = data_out_c;
    if (core.result_c[0]) unpacked_c = unpacked_c >> 8;
    if (core.result_c[1]) unpacked_c = unpacked_c >> 16;
    switch (f3_c) {
      case 0:  unpacked_c = sign_extend<32>( b8(unpacked_c)); break;
      case 1:  unpacked_c = sign_extend<32>(b16(unpacked_c)); break;
      case 4:  unpacked_c = zero_extend<32>( b8(unpacked_c)); break;
      case 5:  unpacked_c = zero_extend<32>(b16(unpacked_c)); break;
    }

    core.next_wb_addr_d = cat(b5(core.hart_c), rd_c);
    core.next_wb_data_d = op_c == RV32I::OP_LOAD ? unpacked_c : core.result_c;
    core.next_wb_wren_d = op_c != RV32I::OP_STORE && op_c != RV32I::OP_BRANCH;

    if (op_c == RV32I::OP_CUSTOM0 && f3_c == 0) {
      // Swap result and the PC that we'll use to fetch.
      // Execute phase should've deposited the new PC in result
      core.next_wb_data_d = temp_pc_a;
      core.next_hart_a    = core.addr_c;
      temp_pc_a      = core.result_c;
    }

    if (regfile_cs_c && op_c == RV32I::OP_STORE) {
      // Thread writing to other thread's regfile
      core.next_wb_addr_d = b10(core.addr_c >> 2);
      core.next_wb_data_d = core.result_c;
      core.next_wb_wren_d = 1;
    }

    //----------
    // Code/data/reg read/write overrides for cross-thread stuff

    // Hmm we can't actually read from code because we also have to read our next instruction
    // and we can't do it earlier or later (we can read it during C, but then it's not back
    // in time to write to the regfile).

    logic<1>  code_wren_c = (op_c == RV32I::OP_STORE) && code_cs_c;
    logic<1>  data_wren_b = (op_b == RV32I::OP_STORE) && data_cs_b;

    logic<32> code_addr_c = code_cs_c ? core.addr_c : temp_pc_a;
    logic<32> data_addr_b = addr_b;

    logic<10> reg_raddr1_a = cat(b5(core.hart_a), rs1a_a);
    logic<10> reg_raddr2_a = cat(b5(core.hart_a), rs2a_a);
    logic<1>  regfile_wren_b = (op_b == RV32I::OP_STORE) && regfile_cs_b;

    if ((op_b == RV32I::OP_LOAD) && regfile_cs_b && (core.pc_a == 0)) {
      reg_raddr1_a = b10(addr_b >> 2);
    }

    //----------
    // Submod tocks

    code.tock(b12(code_addr_c), core.result_c, temp_mask_c, code_wren_c);
    data_ram.tock(b12(data_addr_b), rs2_b,    temp_mask_b, data_wren_b);
    core.regs.tock(reg_raddr1_a, reg_raddr2_a, core.next_wb_addr_d, core.next_wb_data_d, core.next_wb_wren_d);

    // metron_noconvert
    {
      console1.tock(console1_cs_b && op_b == RV32I::OP_STORE, rs2_b);
      console2.tock(console2_cs_b && op_b == RV32I::OP_STORE, rs2_b);
      console3.tock(console3_cs_b && op_b == RV32I::OP_STORE, rs2_b);
      console4.tock(console4_cs_b && op_b == RV32I::OP_STORE, rs2_b);
    }

    //----------
    // Signal writeback

    next_debug_reg = (op_b == RV32I::OP_STORE) && debug_cs_b ? rs2_b : debug_reg;

    core.next_pc_a = temp_pc_a;
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

  pinwheel_core core;

  // metron_internal

  logic<32> next_debug_reg;
  logic<32> debug_reg;

  logic<32> gpio_dir;
  logic<32> gpio_in;
  logic<32> gpio_out;

  block_ram  code;

  // FIXME having this named data and a field inside block_ram named data breaks context resolve
  block_ram  data_ram;

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
