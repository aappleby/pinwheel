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

struct vane {
  logic<5>  hart;
  logic<32> pc;
  logic<32> insn;
  logic<1>  enable;
  logic<1>  active;
};

//------------------------------------------------------------------------------

class Pinwheel {
 public:
  void tock(logic<1> reset) {
    tick(reset);
  }

  static const int hart_count = 4;
  static const int vane_count = 4;

  uint32_t code_mem[16384];  // Cores share ROM
  logic<32> code;

  uint32_t data_mem[16384];  // Cores share RAM
  logic<32> data;

  // Cores have their own register files
  uint32_t regfile[1024];
  logic<32> reg_a;
  logic<32> reg_b;

  // Copy of address, used to realign data after read
  logic<32> reg_addr;

  // Copy of alu output, used for register writeback
  logic<32> reg_alu;

  vane vane0;
  vane vane1;
  vane vane2;

  //----------------------------------------

  void reset() {
    memset(this, 0, sizeof(*this));

    std::string s;
    value_plusargs("text_file=%s", s);
    readmemh(s, code_mem);

    value_plusargs("data_file=%s", s);
    readmemh(s, data_mem);

    vane0.hart   = 0;
    vane1.hart   = 1;
    vane2.hart   = 2;

    vane0.enable = 1;
  }

  //--------------------------------------------------------------------------------

  static logic<32> unpack(logic<32> insn, logic<32> addr, logic<32> data) {
    logic<2> align = b2(addr);
    logic<3> f3 = b3(insn, 12);

    switch (f3) {
      case 0:  return sign_extend<32>( b8(data, align << 3)); break;
      case 1:  return sign_extend<32>(b16(data, align << 3)); break;
      case 2:  return data; break;
      case 3:  return data; break;
      case 4:  return zero_extend<32>( b8(data, align << 3)); break;
      case 5:  return zero_extend<32>(b16(data, align << 3)); break;
      case 6:  return data; break;
      case 7:  return data; break;
      default: return 0;
    }
  }

  //--------------------------------------------------------------------------------

  static logic<32> alu(logic<32> insn, logic<32> pc, logic<32> reg_a, logic<32> reg_b) {
    logic<5> op  = b5(insn, 2);
    logic<3> f3  = b3(insn, 12);
    logic<1> alt = b1(insn, 30);

    if (op == OP_ALU && f3 == 0 && alt) reg_b = -reg_b;

    logic<32> imm_i = sign_extend<32>(b12(insn, 20));
    logic<32> imm_u = b20(insn, 12) << 12;

    logic<3>  alu_op;
    logic<32> a, b;

    switch(op) {
      case OP_ALU:     alu_op = f3;  a = reg_a; b = reg_b;   break;
      case OP_ALUI:    alu_op = f3;  a = reg_a; b = imm_i;   break;
      case OP_LOAD:    alu_op = f3;  a = reg_a; b = reg_b;   break;
      case OP_STORE:   alu_op = f3;  a = reg_a; b = reg_b;   break;
      case OP_BRANCH:  alu_op = f3;  a = reg_a; b = reg_b;   break;
      case OP_JAL:     alu_op = 0;   a = pc;    b = b32(4);  break;
      case OP_JALR:    alu_op = 0;   a = pc;    b = b32(4);  break;
      case OP_LUI:     alu_op = 0;   a = 0;     b = imm_u;   break;
      case OP_AUIPC:   alu_op = 0;   a = pc;    b = imm_u;   break;
      default:         alu_op = f3;  a = 0;     b = imm_u;   break;
    }

    switch (alu_op) {
      case 0:  return a + b; break;
      case 1:  return a << b5(b); break;
      case 2:  return signed(a) < signed(b); break;
      case 3:  return a < b; break;
      case 4:  return a ^ b; break;
      case 5:  return alt ? signed(a) >> b5(b) : a >> b5(b); break;
      case 6:  return a | b; break;
      case 7:  return a & b; break;
      default: return 0;
    }
  }

  //--------------------------------------------------------------------------------

  static logic<1> take_branch(logic<32> insn, logic<32> reg_a, logic<32> reg_b) {
    logic<1> eq  = reg_a == reg_b;
    logic<1> slt = signed(reg_a) < signed(reg_b);
    logic<1> ult = reg_a < reg_b;

    logic<3> f3 = b3(insn, 12);
    switch (f3) {
      case 0:  return   eq;
      case 1:  return  !eq;
      case 2:  return   eq;
      case 3:  return  !eq;
      case 4:  return  slt;
      case 5:  return !slt;
      case 6:  return  ult;
      case 7:  return !ult;
      default: return 0;
    }
  }

  //--------------------------------------------------------------------------------

  static logic<32> pc_gen(logic<32> pc, logic<32> insn, logic<1> active, logic<1> take_branch, logic<32> a) {
    logic<5> op = b5(insn, 2);

    if (!active) {
      return 0;
    }
    else if (op == OP_BRANCH) {
      logic<32> imm_b = cat(dup<20>(insn[31]), insn[7], b6(insn, 25), b4(insn, 8), b1(0));
      return pc + (take_branch ? imm_b : b32(4));
    }
    else if (op == OP_JAL) {
      logic<32> imm_j = cat(dup<12>(insn[31]), b8(insn, 12), insn[20], b10(insn, 21), b1(0));
      return pc + imm_j;
    }
    else if (op == OP_JALR) {
      logic<32> imm_i = sign_extend<32>(b12(insn, 20));
      return a + imm_i;
    }
    else {
      return pc + 4;
    }
  }

  //--------------------------------------------------------------------------------

  static logic<32> addr_gen(logic<32> insn, logic<32> reg_a) {
    logic<5>  op    = b5(insn, 2);
    logic<32> imm_i = sign_extend<32>(b12(insn, 20));
    logic<32> imm_s = cat(dup<21>(insn[31]), b6(insn, 25), b5(insn, 7));
    logic<32> addr  = reg_a + ((op == OP_STORE) ? imm_s : imm_i);
    return addr;
  }

  //--------------------------------------------------------------------------------

  static logic<4> mask_gen(logic<32> insn, logic<32> addr) {
    logic<2> align = b2(addr);
    logic<5> op    = b5(insn, 2);
    logic<3> f3    = b3(insn, 12);

    if (op == OP_STORE) {
      if      (f3 == 0) return 0b0001 << align;
      else if (f3 == 1) return 0b0011 << align;
      else if (f3 == 2) return 0b1111;
      else              return 0;
    }
    else {
      return 0;
    }
  }

  //--------------------------------------------------------------------------------

  void tick(logic<1> reset_in) {
    if (reset_in) {
      reset();
      return;
    }

    const auto code     = this->code;
    const auto data     = this->data;
    const auto reg_a    = this->reg_a;
    const auto reg_b    = this->reg_b;
    const auto reg_addr = this->reg_addr;
    const auto reg_alu  = this->reg_alu;
    const auto vane0    = this->vane0;
    const auto vane1    = this->vane1;
    const auto vane2    = this->vane2;

    //----------

    this->vane0 = vane2;
    this->vane0.active = vane2.enable | vane2.active;

    this->vane1 = vane0;
    this->vane1.insn = code;

    this->vane2 = vane1;

    {
      logic<1> branch = take_branch(vane1.insn, reg_a, reg_b);
      this->vane2.pc = pc_gen(vane1.pc, vane1.insn, vane1.active, branch, reg_a);
    }

    {
      logic<32> addr = addr_gen(vane1.insn, reg_a);
      logic<4>  mask = mask_gen(vane1.insn, addr);
      this->tick_dbus(addr, reg_b, mask);
      this->reg_addr = addr;
    }

    this->tick_pbus(vane2.pc);

    {
      logic<10> raddr1 = cat(vane0.hart, b5(code, 15));
      logic<10> raddr2 = cat(vane0.hart, b5(code, 20));

      auto v2_op = b5(vane2.insn, 2);
      logic<10> waddr  = cat(vane2.hart, b5(vane2.insn, 7));
      logic<1>  wren   = waddr != 0 && v2_op != OP_STORE && v2_op != OP_BRANCH;
      logic<32> wdata  = v2_op == OP_LOAD ? unpack(vane2.insn, reg_addr, data) : reg_alu;

      this->tick_regfile(raddr1, raddr2, waddr, wren, wdata);
    }

    this->reg_alu = alu(vane1.insn, vane1.pc, reg_a, reg_b);
  }

  //--------------------------------------------------------------------------------

  void tick_pbus(logic<32> addr) {
    code = code_mem[b10(addr, 2)];
  }

  //--------------------------------------------------------------------------------

  void tick_dbus(logic<32> addr, logic<32> wdata, logic<4> wmask) {

    logic<2> align = b2(addr);
    wdata = wdata << (8 * align);

    logic<32> r2_data = data_mem[b10(addr, 2)];
    if (wmask) {
      if (addr != 0x40000000) {
        if (wmask[0]) r2_data = (r2_data & 0xFFFFFF00) | (wdata & 0x000000FF);
        if (wmask[1]) r2_data = (r2_data & 0xFFFF00FF) | (wdata & 0x0000FF00);
        if (wmask[2]) r2_data = (r2_data & 0xFF00FFFF) | (wdata & 0x00FF0000);
        if (wmask[3]) r2_data = (r2_data & 0x00FFFFFF) | (wdata & 0xFF000000);
        data_mem[b10(addr, 2)] = r2_data;
      }
    }
    data = r2_data;
  }

  //--------------------------------------------------------------------------------

  void tick_regfile(logic<10> raddr1, logic<10> raddr2, logic<10> waddr, logic<1> wren, logic<32> wdata) {
    if (wren) regfile[waddr] = wdata;
    reg_a = regfile[raddr1];
    reg_b = regfile[raddr2];
  }
};
