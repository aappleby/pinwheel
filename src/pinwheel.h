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

inline const char* op_id_to_name(int op_id) {
  switch(op_id) {
    case OP_ALU:    return "OP_ALU   ";
    case OP_ALUI:   return "OP_ALUI  ";
    case OP_LOAD:   return "OP_LOAD  ";
    case OP_STORE:  return "OP_STORE ";
    case OP_BRANCH: return "OP_BRANCH";
    case OP_JAL:    return "OP_JAL   ";
    case OP_JALR:   return "OP_JALR  ";
    case OP_LUI:    return "OP_LUI   ";
    case OP_AUIPC:  return "OP_AUIPC ";
    default:        return "???      ";
  }
}

//------------------------------------------------------------------------------

struct registers_p0 {
  logic<5>  hart;
  logic<32> pc;
};

struct registers_p1 {
  logic<5>  hart;
  logic<32> pc;
  logic<32> insn;
};

struct registers_p2 {
  logic<5>  hart;
  logic<32> pc;
  logic<32> insn;

  logic<5>  align;
  logic<32> rbus_wdata;
};

//------------------------------------------------------------------------------

class Pinwheel {
 public:
  void tock(logic<1> reset) {
    tick(reset);
  }

  uint32_t code_mem[16384];  // Cores share a 4K ROM
  uint32_t data_mem[16384];  // Cores share a 4K RAM
  uint32_t regfile[32][32]; // Cores have their own register file

  registers_p0 reg_p0;
  registers_p1 reg_p1;
  registers_p2 reg_p2;

  logic<5>  rbus_raddr1;
  logic<5>  rbus_raddr2;
  logic<5>  rbus_waddr;
  logic<32> rbus_wdata;

  logic<32> dbus_addr;
  logic<32> dbus_wdata;
  logic<4>  dbus_mask;

  void dump() {
    if (reg_p0.hart == 0) {
      //printf("h%dp0: 0x%08x\n", (int)reg_p0.hart, (int)reg_p0.pc);
    }
    if (reg_p1.hart == 0) {
      //printf("h%dp1: 0x%08x %s inst 0x%08x imm 0x%08x\n", (int)reg_p1.hart, (int)reg_p1.pc, op_id_to_name(reg_p1.op), (uint32_t)reg_p1.insn, (uint32_t)reg_p1.imm);
    }
    if (reg_p2.hart == 0) {
      //printf("h%dp2: 0x%08x %s alu 0x%08x\n", (int)reg_p2.hart, (int)reg_p2.pc, op_id_to_name(reg_p2.op), (uint32_t)reg_p2.alu);
    }
  }

  //----------------------------------------

  void reset() {
    std::string s;
    value_plusargs("text_file=%s", s);
    readmemh(s, code_mem);

    value_plusargs("data_file=%s", s);
    readmemh(s, data_mem);

    memset(regfile, 0, sizeof(regfile));

    reg_p0.hart   = 0;
    reg_p0.pc     = 0;

    reg_p1.hart   = 0;
    reg_p1.pc     = 0;
    reg_p1.insn   = 0;

    reg_p2.hart   = 1;
    reg_p2.pc     = -4;
    reg_p2.insn   = 0;

    rbus_raddr1 = 0;
    rbus_raddr2 = 0;
    rbus_waddr  = 0;
    rbus_wdata  = 0;

    dbus_addr  = 0;
    dbus_wdata = 0;
    dbus_mask  = 0;
  }

  //--------------------------------------------------------------------------------

  void tick(logic<1> reset_in) {
    if (reset_in) {
      reset();
      return;
    }

    logic<32> pbus_data = code_mem[b10(reg_p0.pc, 2)];
    logic<32> dbus_data = data_mem[b10(dbus_addr, 2)];
    if (dbus_mask) {
      if (dbus_addr != 0x40000000) {
        logic<32> dbus_data = data_mem[b10(dbus_addr, 2)];
        if (dbus_mask[0]) dbus_data = (dbus_data & 0xFFFFFF00) | (dbus_wdata & 0x000000FF);
        if (dbus_mask[1]) dbus_data = (dbus_data & 0xFFFF00FF) | (dbus_wdata & 0x0000FF00);
        if (dbus_mask[2]) dbus_data = (dbus_data & 0xFF00FFFF) | (dbus_wdata & 0x00FF0000);
        if (dbus_mask[3]) dbus_data = (dbus_data & 0x00FFFFFF) | (dbus_wdata & 0xFF000000);
        data_mem[b10(dbus_addr, 2)] = dbus_data;
      }
    }

    // Regfile
    logic<32> ra = regfile[reg_p1.hart][rbus_raddr1];
    logic<32> rb = regfile[reg_p1.hart][rbus_raddr2];
    if (rbus_waddr) {
      regfile[reg_p0.hart][rbus_waddr] = rbus_wdata;
    }

    auto old_reg_p0 = reg_p0;
    auto old_reg_p1 = reg_p1;
    auto old_reg_p2 = reg_p2;

    //----------

    rbus_raddr1 = b5(pbus_data, 15);
    rbus_raddr2 = b5(pbus_data, 20);

    reg_p1.hart = old_reg_p0.hart;
    reg_p1.pc   = old_reg_p0.pc;
    reg_p1.insn = pbus_data;

    //----------

    logic<32> p1_insn = old_reg_p1.insn;
    logic<5>  p1_op = b5(p1_insn, 2);
    logic<3>  p1_f3 = b3(p1_insn, 12);
    logic<32> p1_imm_b = cat(dup<20>(p1_insn[31]), p1_insn[7], b6(p1_insn, 25), b4(p1_insn, 8), b1(0));
    logic<32> p1_imm_i = cat(dup<21>(p1_insn[31]), b6(p1_insn, 25), b5(p1_insn, 20));
    logic<32> p1_imm_j = cat(dup<12>(p1_insn[31]), b8(p1_insn, 12), p1_insn[20], b6(p1_insn, 25), b4(p1_insn, 21), b1(0));
    logic<32> p1_imm_u = cat(p1_insn[31], b11(p1_insn, 20), b8(p1_insn, 12), b12(0));
    logic<32> p1_imm_s = cat(dup<21>(p1_insn[31]), b6(p1_insn, 25), b5(p1_insn, 7));

    // Data bus driver
    logic<32> p1_addr = ra + ((p1_op == OP_STORE) ? p1_imm_s : p1_imm_i);

    logic<2>  p1_align = b2(p1_addr);
    dbus_addr  = p1_addr;
    dbus_wdata = rb << (8 * p1_align);
    dbus_mask  = 0b0000;

    if (p1_op == OP_STORE) {
      logic<3> f3 = b3(p1_insn, 12);
      if      (f3 == 0) dbus_mask = 0b0001 << p1_align;
      else if (f3 == 1) dbus_mask = 0b0011 << p1_align;
      else if (f3 == 2) dbus_mask = 0b1111;
      else              dbus_mask = 0b0000;
    }

    if (!old_reg_p1.hart) {
      dbus_addr  = 0;
      dbus_wdata = 0;
      dbus_mask  = 0;
    }

    // ALU
    logic<1>  p1_alu_alt = b1(old_reg_p1.insn, 30);
    logic<32> p1_alu_a  = ra;
    logic<32> p1_alu_b  = rb;
    logic<3>  p1_alu_op = p1_f3;

    switch(p1_op) {
      case OP_ALU:    p1_alu_op = p1_f3;     p1_alu_a = ra;              p1_alu_b = (p1_f3 == 0 && p1_alu_alt) ? b32(-rb) : rb; break;
      case OP_ALUI:   p1_alu_op = p1_f3;     p1_alu_a = ra;              p1_alu_b = p1_imm_i;                                   break;
      case OP_LOAD:   p1_alu_op = p1_f3;     p1_alu_a = ra;              p1_alu_b = rb;                                         break;
      case OP_STORE:  p1_alu_op = p1_f3;     p1_alu_a = ra;              p1_alu_b = rb;                                         break;
      case OP_BRANCH: p1_alu_op = p1_f3;     p1_alu_a = ra;              p1_alu_b = rb;                                         break;
      case OP_JAL:    p1_alu_op = 0;         p1_alu_a = old_reg_p1.pc;   p1_alu_b = b32(4);                                     break;
      case OP_JALR:   p1_alu_op = 0;         p1_alu_a = old_reg_p1.pc;   p1_alu_b = b32(4);                                     break;
      case OP_LUI:    p1_alu_op = 0;         p1_alu_a = 0;               p1_alu_b = p1_imm_u;                                   break;
      case OP_AUIPC:  p1_alu_op = 0;         p1_alu_a = old_reg_p1.pc;   p1_alu_b = p1_imm_u;                                   break;
    }

    logic<1> p1_bit = (p1_alu_op == 5) && p1_alu_alt ? p1_alu_a[31] : b1(0);

    logic<32> p1_sl = p1_alu_a;
    if (p1_alu_b[4]) p1_sl = cat(b16(p1_sl, 0), dup<16>(p1_bit));
    if (p1_alu_b[3]) p1_sl = cat(b24(p1_sl, 0), dup< 8>(p1_bit));
    if (p1_alu_b[2]) p1_sl = cat(b28(p1_sl, 0), dup< 4>(p1_bit));
    if (p1_alu_b[1]) p1_sl = cat(b30(p1_sl, 0), dup< 2>(p1_bit));
    if (p1_alu_b[0]) p1_sl = cat(b31(p1_sl, 0), dup< 1>(p1_bit));

    logic<32> p1_sr = p1_alu_a;
    if (p1_alu_b[4]) p1_sr = cat(dup<16>(p1_bit), b16(p1_sr, 16));
    if (p1_alu_b[3]) p1_sr = cat(dup< 8>(p1_bit), b24(p1_sr,  8));
    if (p1_alu_b[2]) p1_sr = cat(dup< 4>(p1_bit), b28(p1_sr,  4));
    if (p1_alu_b[1]) p1_sr = cat(dup< 2>(p1_bit), b30(p1_sr,  2));
    if (p1_alu_b[0]) p1_sr = cat(dup< 1>(p1_bit), b31(p1_sr,  1));

    logic<32> p1_alu_out;
    logic<32> p1_pc_next = old_reg_p1.pc + b32(4);

    switch (p1_alu_op) {
      case 0: p1_alu_out = p1_alu_a + p1_alu_b;                 break;
      case 1: p1_alu_out = p1_sl;                               break;
      case 2: p1_alu_out = signed(p1_alu_a) < signed(p1_alu_b); break;
      case 3: p1_alu_out = p1_alu_a < p1_alu_b;                 break;
      case 4: p1_alu_out = p1_alu_a ^ p1_alu_b;                 break;
      case 5: p1_alu_out = p1_sr;                               break;
      case 6: p1_alu_out = p1_alu_a | p1_alu_b;                 break;
      case 7: p1_alu_out = p1_alu_a & p1_alu_b;                 break;
    }

    // jump_rel
    logic<1> p1_eq  = ra == rb;
    logic<1> p1_slt = signed(ra) < signed(rb);
    logic<1> p1_ult = ra < rb;
    logic<1> p1_jump_rel = 0;
    if (p1_op == OP_BRANCH) {
      switch (p1_f3) {
        case 0: p1_jump_rel =   p1_eq; break;
        case 1: p1_jump_rel =  !p1_eq; break;
        case 2: p1_jump_rel =   p1_eq; break;
        case 3: p1_jump_rel =  !p1_eq; break;
        case 4: p1_jump_rel =  p1_slt; break;
        case 5: p1_jump_rel = !p1_slt; break;
        case 6: p1_jump_rel =  p1_ult; break;
        case 7: p1_jump_rel = !p1_ult; break;
      }
    }
    else if (p1_op == OP_JAL)  p1_jump_rel = 1;
    else if (p1_op == OP_JALR) p1_jump_rel = 1;

    // Next PC
    if (p1_op == OP_BRANCH && p1_jump_rel) { p1_pc_next = old_reg_p1.pc + p1_imm_b; }
    if (p1_op == OP_JAL)                   { p1_pc_next = old_reg_p1.pc + p1_imm_j; }
    if (p1_op == OP_JALR)                  { p1_pc_next = ra + p1_imm_i; }

    reg_p2.hart       = old_reg_p1.hart;
    reg_p2.pc         = p1_pc_next;
    reg_p2.insn       = old_reg_p1.insn;
    reg_p2.align      = b2(dbus_addr);
    reg_p2.rbus_wdata = p1_alu_out;

    //----------

    logic<5>  p2_op = b5(old_reg_p2.insn, 2);
    logic<3>  p2_f3 = b3(old_reg_p2.insn, 12);

    // Writeback
    logic<32> p2_unpacked; // Unpack byte/word from memory dword
    switch (p2_f3) {
      case 0: p2_unpacked = sign_extend<32>(b8(dbus_data, 8 * old_reg_p2.align)); break;
      case 1: p2_unpacked = sign_extend<32>(b16(dbus_data, 8 * old_reg_p2.align)); break;
      case 2: p2_unpacked = dbus_data; break;
      case 3: p2_unpacked = dbus_data; break;
      case 4: p2_unpacked = b8(dbus_data, 8 * old_reg_p2.align); break;
      case 5: p2_unpacked = b16(dbus_data, 8 * old_reg_p2.align); break;
      case 6: p2_unpacked = dbus_data; break;
      case 7: p2_unpacked = dbus_data; break;
    }

    rbus_wdata = p2_op == OP_LOAD ? p2_unpacked : old_reg_p2.rbus_wdata;
    rbus_waddr = b5(old_reg_p2.insn, 7);
    if (p2_op == OP_STORE)  rbus_waddr = 0;
    if (p2_op == OP_BRANCH) rbus_waddr = 0;

    // Next PC
    reg_p0.hart = old_reg_p2.hart;
    reg_p0.pc   = old_reg_p2.pc;
  }
};
