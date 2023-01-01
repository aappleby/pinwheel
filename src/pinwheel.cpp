#include "pinwheel.h"

//--------------------------------------------------------------------------------

void BlockRam::tick_read(logic<32> raddr, logic<1> rden) {
  if (rden) {
    out = data[b10(raddr, 2)];
  }
  else {
    out = 0;
  }
}

void BlockRam::tick_write(logic<32> waddr, logic<32> wdata, logic<4> wmask, logic<1> wren) {
  if (wren) {
    logic<32> old_data = data[b10(waddr, 2)];
    logic<32> new_data = wdata << (8 * b2(waddr));
    data[b10(waddr, 2)] = ((wmask[0] ? new_data : old_data) & 0x000000FF) |
                          ((wmask[1] ? new_data : old_data) & 0x0000FF00) |
                          ((wmask[2] ? new_data : old_data) & 0x00FF0000) |
                          ((wmask[3] ? new_data : old_data) & 0xFF000000);
  }
}

//--------------------------------------------------------------------------------

void BlockRegfile::tick_read(logic<10> raddr1, logic<10> raddr2, logic<1> rden) {
  if (rden) {
    out_a = data[raddr1];
    out_b = data[raddr2];
  }
  else {
    out_a = 0;
    out_b = 0;
  }
}

void BlockRegfile::tick_write(logic<10> waddr, logic<32> wdata, logic<1> wren) {
  if (wren) {
    data[waddr] = wdata;
  }
}

//--------------------------------------------------------------------------------

Pinwheel::Pinwheel() {
}

Pinwheel* Pinwheel::clone() {
  Pinwheel* p = new Pinwheel();
  memcpy(p, this, sizeof(*this));
  return p;
}

//--------------------------------------------------------------------------------

void Pinwheel::reset() {
  memset(this, 0, sizeof(*this));

  std::string s;
  value_plusargs("text_file=%s", s);
  readmemh(s, code.data);

  value_plusargs("data_file=%s", s);
  readmemh(s, data.data);

  vane0_pc = 0x00400000;
  vane1_pc = 0x00400000;
  vane2_pc = 0x00400000;

  vane0_hart = 0;
  vane1_hart = 1;
  vane2_hart = 2;

  vane0_enable = 1;

  debug_reg = 0;
}

//--------------------------------------------------------------------------------

logic<32> Pinwheel::unpack(logic<32> insn, logic<32> addr, logic<32> data) {
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

logic<32> Pinwheel::alu(logic<32> insn, logic<32> pc, logic<32> reg_a, logic<32> reg_b) {
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
    case OP_JAL:     return pc + 4;
    case OP_JALR:    return pc + 4;
    case OP_LUI:     return imm_u;
    case OP_AUIPC:   return pc + imm_u;
    default:         return 0;
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

logic<32> Pinwheel::pc_gen(logic<32> pc, logic<32> insn, logic<1> active, logic<32> reg_a, logic<32> reg_b) {
  if (!active) {
    return pc;
  }

  logic<5> op  = b5(insn, 2);
  logic<1> eq  = reg_a == reg_b;
  logic<1> slt = signed(reg_a) < signed(reg_b);
  logic<1> ult = reg_a < reg_b;

  logic<3> f3 = b3(insn, 12);
  logic<1> take_branch;
  switch (f3) {
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

  if (op == OP_BRANCH) {
    logic<32> imm_b = cat(dup<20>(insn[31]), insn[7], b6(insn, 25), b4(insn, 8), b1(0));
    return pc + (take_branch ? imm_b : b32(4));
  }
  else if (op == OP_JAL) {
    logic<32> imm_j = cat(dup<12>(insn[31]), b8(insn, 12), insn[20], b10(insn, 21), b1(0));
    return pc + imm_j;
  }
  else if (op == OP_JALR) {
    logic<32> imm_i = sign_extend<32>(b12(insn, 20));
    return reg_a + imm_i;
  }
  else {
    return pc + 4;
  }
}

//--------------------------------------------------------------------------------

logic<32> Pinwheel::addr_gen(logic<32> insn, logic<32> reg_a) {
  logic<5>  op    = b5(insn, 2);
  logic<32> imm_i = sign_extend<32>(b12(insn, 20));
  logic<32> imm_s = cat(dup<21>(insn[31]), b6(insn, 25), b5(insn, 7));
  logic<32> addr  = reg_a + ((op == OP_STORE) ? imm_s : imm_i);
  return addr;
}

//--------------------------------------------------------------------------------

logic<4> Pinwheel::mask_gen(logic<32> insn, logic<32> addr) {
  logic<5> op = b5(insn, 2);
  if (op != OP_STORE) return 0;

  logic<3> f3 = b3(insn, 12);
  logic<2> align = b2(addr);

  if      (f3 == 0) return 0b0001 << align;
  else if (f3 == 1) return 0b0011 << align;
  else if (f3 == 2) return 0b1111;
  else              return 0;
}

//--------------------------------------------------------------------------------

void Pinwheel::tick(logic<1> reset_in) {
  if (reset_in) {
    reset();
    return;
  }

  ticks++;

  //----------

  const auto vane0_hart   = this->vane0_hart;
  const auto vane0_pc     = this->vane0_pc;
  const auto vane0_enable = this->vane0_enable;
  const auto vane0_active = this->vane0_active;

  const auto vane1_hart   = this->vane1_hart;
  const auto vane1_pc     = this->vane1_pc;
  const auto vane1_insn   = this->vane1_insn;
  const auto vane1_enable = this->vane1_enable;
  const auto vane1_active = this->vane1_active;

  const auto vane2_hart   = this->vane2_hart;
  const auto vane2_pc     = this->vane2_pc;
  const auto vane2_insn   = this->vane2_insn;
  const auto vane2_enable = this->vane2_enable;
  const auto vane2_active = this->vane2_active;

  //----------

  logic<5>  vane1_op = b5(vane1_insn, 2);
  logic<5>  vane2_op = b5(vane2_insn, 2);

  logic<32> vane1_reg_a = b5(vane1_insn, 15) ? regs.out_a : b32(0);
  logic<32> vane1_reg_b = b5(vane1_insn, 20) ? regs.out_b : b32(0);

  logic<32> code_rdata = code.out;
  logic<32> data_rdata = data.out;

  logic<32> bus_rdata = unpack(vane2_insn, vane2_mem_addr, data_rdata);

  logic<32> vane1_mem_addr  = addr_gen(vane1_insn, vane1_reg_a);
  logic<1>  vane1_mem_rden  = vane1_active && vane1_op == OP_LOAD;
  logic<1>  vane1_mem_wren  = vane1_active && vane1_op == OP_STORE;
  logic<32> vane1_mem_wdata = vane1_reg_b;
  logic<4>  vane1_mem_wmask = mask_gen(vane1_insn, vane1_mem_addr);

  logic<1> data_cs    = b4(vane1_mem_addr, 28) == 0x8;
  logic<1> debug_cs   = b4(vane1_mem_addr, 28) == 0xF;

  logic<10> vane2_reg_waddr = cat(vane2_hart, b5(vane2_insn, 7));
  logic<1>  vane2_reg_wren  = (vane2_enable | vane2_active) && vane2_reg_waddr != 0 && vane2_op != OP_STORE && vane2_op != OP_BRANCH;
  logic<32> vane2_reg_wdata = vane2_op == OP_LOAD ? bus_rdata : vane2_alu_out;

  logic<10> vane0_reg_raddr1 = cat(vane0_hart, b5(code_rdata, 15));
  logic<10> vane0_reg_raddr2 = cat(vane0_hart, b5(code_rdata, 20));
  logic<1>  vane0_reg_rden   = vane0_active;

  this->debug_reg = vane1_mem_wren && debug_cs ? vane1_mem_wdata : debug_reg;

  this->data.tick_read (vane1_mem_addr, vane1_mem_rden && data_cs);
  this->data.tick_write(vane1_mem_addr, vane1_mem_wdata, vane1_mem_wmask, vane1_mem_wren && data_cs);

  this->code.tick_read(vane2_pc, true);
  this->regs.tick_write(vane2_reg_waddr, vane2_reg_wdata, vane2_reg_wren);

  this->regs.tick_read(vane0_reg_raddr1, vane0_reg_raddr2, vane0_reg_rden);

  this->vane0_hart   = vane2_hart;
  this->vane0_pc     = vane2_pc;
  this->vane0_enable = vane2_enable;
  this->vane0_active = vane2_enable | vane2_active;

  this->vane1_hart   = vane0_hart;
  this->vane1_pc     = vane0_pc;
  this->vane1_insn   = code_rdata;
  this->vane1_enable = vane0_enable;
  this->vane1_active = vane0_active;

  this->vane2_hart   = vane1_hart;
  this->vane2_pc     = pc_gen(vane1_pc, vane1_insn, vane1_active, vane1_reg_a, vane1_reg_b);
  this->vane2_insn   = vane1_insn;
  this->vane2_enable = vane1_enable;
  this->vane2_active = vane1_active;

  this->vane2_alu_out = alu(vane1_insn, vane1_pc, vane1_reg_a, vane1_reg_b);

  this->vane2_mem_addr = vane1_mem_addr;
}

//--------------------------------------------------------------------------------
