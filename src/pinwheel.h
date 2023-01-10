#pragma once
#include "metron_tools.h"

#include "block_ram.h"
//noconvert
#include "console.h"
#include "constants.h"
#include "regfile.h"

//------------------------------------------------------------------------------

class Pinwheel {
public:

  Pinwheel() {
    std::string s;
    value_plusargs("text_file=%s", s);
    readmemh(s, code.get_data());

    value_plusargs("data_file=%s", s);
    readmemh(s, data.get_data());
  }

  // noconvert
  Pinwheel* clone() {
    Pinwheel* p = new Pinwheel();
    memcpy(p, this, sizeof(*this));
    return p;
  }

  // noconvert
  size_t size_bytes() {
    return sizeof(*this);
  }

  //----------

  /*
  void reset_mem() {
    memset(&code,    0x00, sizeof(code));
    memset(&data,    0x00, sizeof(data));
    memset(&regfile, 0,    sizeof(regfile));
  }
  */

  //----------

  static logic<32> decode_imm(logic<32> insn) {
    logic<5>  op    = b5(insn, 2);
    logic<32> imm_i = sign_extend<32>(b12(insn, 20));
    logic<32> imm_s = cat(dup<21>(insn[31]), b6(insn, 25), b5(insn, 7));
    logic<32> imm_u = b20(insn, 12) << 12;
    logic<32> imm_b = cat(dup<20>(insn[31]), insn[7], b6(insn, 25), b4(insn, 8), b1(0));
    logic<32> imm_j = cat(dup<12>(insn[31]), b8(insn, 12), insn[20], b10(insn, 21), b1(0));

    logic<32> result;
    switch(op) {
      case RV32I::OP_LOAD:   result = imm_i; break;
      case RV32I::OP_OPIMM:  result = imm_i; break;
      case RV32I::OP_AUIPC:  result = imm_u; break;
      case RV32I::OP_STORE:  result = imm_s; break;
      case RV32I::OP_OP:     result = imm_i; break;
      case RV32I::OP_LUI:    result = imm_u; break;
      case RV32I::OP_BRANCH: result = imm_b; break;
      case RV32I::OP_JALR:   result = imm_i; break;
      case RV32I::OP_JAL:    result = imm_j; break;
      default:              result = 0;     break;
    }
    return result;
  }

  //----------

  logic<32> execute_alu(logic<32> insn, logic<32> reg_a, logic<32> reg_b) const {
    logic<5>  op  = b5(insn, 2);
    logic<3>  f3  = b3(insn, 12);
    logic<7>  f7  = b7(insn, 25);
    logic<32> imm = decode_imm(insn);

    logic<32> alu_a = reg_a;
    logic<32> alu_b = op == RV32I::OP_OPIMM ? imm : reg_b;
    if (op == RV32I::OP_OP && f3 == 0 && f7 == 32) alu_b = -alu_b;

    logic<32> result;
    switch (f3) {
      case 0:  result = alu_a + alu_b; break;
      case 1:  result = alu_a << b5(alu_b); break;
      case 2:  result = signed(alu_a) < signed(alu_b); break;
      case 3:  result = alu_a < alu_b; break;
      case 4:  result = alu_a ^ alu_b; break;
      case 5:  result = f7 == 32 ? signed(alu_a) >> b5(alu_b) : alu_a >> b5(alu_b); break;
      case 6:  result = alu_a | alu_b; break;
      case 7:  result = alu_a & alu_b; break;
      default: result = 0;
    }
    return result;
  }

  //----------

  logic<32> execute_system(logic<32> insn) const {
    logic<3>  f3  = b3(insn, 12);
    logic<12> csr = b12(insn, 20);

    logic<32> result = 0;
    switch(f3) {
      case 0:               break;
      case RV32I::F3_CSRRW:  break;
      case RV32I::F3_CSRRS:  if (csr == 0xF14) result = hart_b; break;
      case RV32I::F3_CSRRC:  break;
      case 4:               break;
      case RV32I::F3_CSRRWI: break;
      case RV32I::F3_CSRRSI: break;
      case RV32I::F3_CSRRCI: break;
    }
    return result;
  }

  //----------

  void tock_twocycle(logic<1> reset_in) {
    logic<5> op_b   = b5(insn_b, 2);
    logic<5> rda_b  = b5(insn_b, 7);
    logic<3> f3_b   = b3(insn_b, 12);
    logic<5> rs1a_b = b5(insn_b, 15);
    logic<5> rs2a_b = b5(insn_b, 20);
    logic<7> f7_b   = b7(insn_b, 25);

    logic<5> op_c   = b5(insn_c, 2);
    logic<5> rd_c   = b5(insn_c, 7);
    logic<3> f3_c   = b3(insn_c, 12);

    logic<32> rs1_b  = rs1a_b ? regs.get_rs1() : b32(0);
    logic<32> rs2_b  = rs2a_b ? regs.get_rs2() : b32(0);
    logic<32> imm_b  = decode_imm(insn_b);
    logic<32> addr_b = b32(rs1_b + imm_b);

    logic<32> temp_pc_a = 0;

    next_hart_a    = hart_b;
    next_pc_a      = 0;
    next_insn_b    = 0;
    next_addr_c    = 0;
    next_result_c  = 0;
    next_wb_addr_d = 0;
    next_wb_data_d = 0;
    next_wb_wren_d = 0;
    next_debug_reg = 0;

    //----------
    // Fetch

    {
      if (pc_b) {
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
          case RV32I::OP_BRANCH:  temp_pc_a = take_branch ? pc_b + imm_b : pc_b + b32(4); break;
          case RV32I::OP_JAL:     temp_pc_a = pc_b + imm_b; break;
          case RV32I::OP_JALR:    temp_pc_a = addr_b; break;
          default:                temp_pc_a = pc_b + 4; break;
        }
      }
    }

    //----------
    // Decode

    logic<32> insn_a = code.rdata();
    logic<5> rs1a_a  = b5(insn_a, 15);
    logic<5> rs2a_a  = b5(insn_a, 20);

    next_insn_b = pc_a == 0 ? b32(0) : insn_a;
    next_addr_c = addr_b;

    //----------
    // Execute

    switch(op_b) {
      case RV32I::OP_JAL:     next_result_c = pc_b + 4;     break;
      case RV32I::OP_JALR:    next_result_c = pc_b + 4;     break;
      case RV32I::OP_LUI:     next_result_c = imm_b;        break;
      case RV32I::OP_AUIPC:   next_result_c = pc_b + imm_b; break;
      case RV32I::OP_LOAD:    next_result_c = addr_b;       break;
      case RV32I::OP_STORE:   next_result_c = rs2_b;        break;
      case RV32I::OP_CUSTOM0: {
        if (f3_b == 0) {
          // Switch the other thread to another hart
          next_addr_c   = rs1_b;
          next_result_c = rs2_b;
        }
        else if (f3_b == 1) {
          // Yield to another hart
          next_result_c  = temp_pc_a;
          next_hart_a    = rs1_b;
          temp_pc_a      = rs2_b;
        }
        break;
      }
      case RV32I::OP_SYSTEM:  next_result_c = execute_system(insn_b); break;
      default:               next_result_c = execute_alu   (insn_b, rs1_b, rs2_b); break;
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
    if (addr_c[0]) temp_mask_c = temp_mask_c << 1;
    if (addr_c[1]) temp_mask_c = temp_mask_c << 2;

    next_debug_reg = (op_b == RV32I::OP_STORE) && debug_cs_b ? rs2_b : debug_reg;

    //----------
    // Write

    logic<1> code_cs_c     = b4(addr_c, 28) == 0x0 && temp_pc_a == 0;
    logic<1> console1_cs_c = b4(addr_c, 28) == 0x4;
    logic<1> console2_cs_c = b4(addr_c, 28) == 0x5;
    logic<1> console3_cs_c = b4(addr_c, 28) == 0x6;
    logic<1> console4_cs_c = b4(addr_c, 28) == 0x7;
    logic<1> data_cs_c     = b4(addr_c, 28) == 0x8;
    logic<1> debug_cs_c    = b4(addr_c, 28) == 0xF;
    logic<1> regfile_cs_c  = b4(addr_c, 28) == 0xE;

    logic<32>              data_out_c = 0;
    if      (data_cs_c)    data_out_c = data.rdata();
    else if (debug_cs_c)   data_out_c = debug_reg;
    else if (regfile_cs_c) data_out_c = regs.get_rs1();

    logic<32>        unpacked_c = data_out_c;
    if (result_c[0]) unpacked_c = unpacked_c >> 8;
    if (result_c[1]) unpacked_c = unpacked_c >> 16;
    switch (f3_c) {
      case 0:  unpacked_c = sign_extend<32>( b8(unpacked_c)); break;
      case 1:  unpacked_c = sign_extend<32>(b16(unpacked_c)); break;
      case 4:  unpacked_c = zero_extend<32>( b8(unpacked_c)); break;
      case 5:  unpacked_c = zero_extend<32>(b16(unpacked_c)); break;
    }

    next_wb_addr_d = cat(b5(hart_c), rd_c);
    next_wb_data_d = op_c == RV32I::OP_LOAD ? unpacked_c : result_c;
    next_wb_wren_d = op_c != RV32I::OP_STORE && op_c != RV32I::OP_BRANCH;

    if (op_c == RV32I::OP_CUSTOM0 && f3_c == 0) {
      // Swap result and the PC that we'll use to fetch.
      // Execute phase should've deposited the new PC in result
      next_wb_data_d = temp_pc_a;
      next_hart_a    = addr_c;
      temp_pc_a      = result_c;
    }

    if (regfile_cs_c && op_c == RV32I::OP_STORE) {
      // Thread writing to other thread's regfile
      next_wb_addr_d = b10(addr_c >> 2);
      next_wb_data_d = result_c;
      next_wb_wren_d = 1;
    }

    //----------
    // Code/data/reg read/write overrides for cross-thread stuff

    // Hmm we can't actually read from code because we also have to read our next instruction
    // and we can't do it earlier or later (we can read it during C, but then it's not back
    // in time to write to the regfile).

    logic<1>  code_wren_c = (op_c == RV32I::OP_STORE) && code_cs_c;
    logic<1>  data_wren_b = (op_b == RV32I::OP_STORE) && data_cs_b;

    logic<32> code_addr_c = code_cs_c ? addr_c : temp_pc_a;
    logic<32> data_addr_b = addr_b;

    logic<10> reg_raddr1_a = cat(b5(hart_a), rs1a_a);
    logic<10> reg_raddr2_a = cat(b5(hart_a), rs2a_a);
    logic<1>  regfile_wren_b = (op_b == RV32I::OP_STORE) && regfile_cs_b;

    if ((op_b == RV32I::OP_LOAD) && regfile_cs_b && (pc_a == 0)) {
      reg_raddr1_a = b10(addr_b >> 2);
    }

    //----------
    // Submod tocks

    code.tock(b12(code_addr_c), result_c, temp_mask_c, code_wren_c);
    data.tock(b12(data_addr_b), rs2_b,    temp_mask_b, data_wren_b);

    regs.tock(reg_raddr1_a, reg_raddr2_a, next_wb_addr_d, next_wb_data_d, next_wb_wren_d);

    // noconvert
    {
      console1.tock(console1_cs_b && op_b == RV32I::OP_STORE, rs2_b);
      console2.tock(console2_cs_b && op_b == RV32I::OP_STORE, rs2_b);
      console3.tock(console3_cs_b && op_b == RV32I::OP_STORE, rs2_b);
      console4.tock(console4_cs_b && op_b == RV32I::OP_STORE, rs2_b);
    }

    //----------
    // Signal writeback

    next_pc_a = temp_pc_a;
  }

  //----------------------------------------

  void tick_twocycle(logic<1> reset_in) {
    if (reset_in) {
      //reset_mem();
      hart_a    = 1;
      pc_a      = 0;

      hart_b    = 0;
      pc_b      = 0x00400000 - 4;
      insn_b    = 0;

      hart_c    = 0;
      pc_c      = 0;
      insn_c    = 0;
      addr_c    = 0;
      result_c  = 0;

      hart_d    = 0;
      pc_d      = 0;
      insn_d    = 0;
      result_d  = 0;
      wb_addr_d = 0;
      wb_data_d = 0;
      wb_wren_d = 0;

      debug_reg = 0;
      // noconvert
      ticks     = 0;
    }
    else {
      hart_d    = hart_c;
      pc_d      = pc_c;
      insn_d    = insn_c;
      result_d  = result_c;
      wb_addr_d = next_wb_addr_d;
      wb_data_d = next_wb_data_d;
      wb_wren_d = next_wb_wren_d;

      hart_c    = hart_b;
      pc_c      = pc_b;
      insn_c    = insn_b;
      addr_c    = next_addr_c;
      result_c  = next_result_c;

      hart_b    = hart_a;
      pc_b      = pc_a;
      insn_b    = next_insn_b;

      hart_a    = next_hart_a;
      pc_a      = next_pc_a;

      debug_reg = next_debug_reg;
      // noconvert
      ticks     = ticks + 1;
    }

    code.tick();
    data.tick();
    regs.tick();

    // noconvert
    console1.tick(reset_in);
    // noconvert
    console2.tick(reset_in);
    // noconvert
    console3.tick(reset_in);
    // noconvert
    console4.tick(reset_in);
  }

  //----------------------------------------

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

  block_ram  code;
  block_ram  data;
  regfile   regs;

  // noconvert
  Console console1;
  // noconvert
  Console console2;
  // noconvert
  Console console3;
  // noconvert
  Console console4;

  // noconvert
  uint64_t ticks;
};

//------------------------------------------------------------------------------
