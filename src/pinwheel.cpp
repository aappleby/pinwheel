#include "pinwheel.h"

//--------------------------------------------------------------------------------

void BlockRam::tick(logic<32> addr, logic<32> wdata, logic<4> wmask) {
  logic<2> align = b2(addr);
  wdata = wdata << (8 * align);

  logic<32> r2_data = data[b10(addr, 2)];
  if (wmask) {
    if (addr != 0x40000000) {
      if (wmask[0]) r2_data = (r2_data & 0xFFFFFF00) | (wdata & 0x000000FF);
      if (wmask[1]) r2_data = (r2_data & 0xFFFF00FF) | (wdata & 0x0000FF00);
      if (wmask[2]) r2_data = (r2_data & 0xFF00FFFF) | (wdata & 0x00FF0000);
      if (wmask[3]) r2_data = (r2_data & 0x00FFFFFF) | (wdata & 0xFF000000);
      data[b10(addr, 2)] = r2_data;
    }
  }
  out = r2_data;
}

//--------------------------------------------------------------------------------

void BlockRegfile::tick(logic<10> raddr1, logic<10> raddr2, logic<10> waddr, logic<1> wren, logic<32> wdata) {
  /*
  if (raddr1 >= 32) {
    printf("BlockRegfile::tick() - reading %d\n", (int)raddr1);
  }
  */

  /*
  if (b5(raddr1) == 0) {
    printf("hart %d reading r0 on port 1\n", (int)b2(raddr1, 5));
  }

  if (b5(raddr2) == 0) {
    printf("hart %d reading r0 on port 2\n", (int)b2(raddr2, 5));
  }
  */

  if (wren) data[waddr] = wdata;
  out_a = data[raddr1];
  out_b = data[raddr2];
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

  vane0.pc = 0x00400000;
  vane1.pc = 0x00400000;
  vane2.pc = 0x00400000;

  vane0.hart   = 0;
  vane1.hart   = 1;
  vane2.hart   = 2;

  //regs.data[ 0] = 0xCAFEBABE;
  //regs.data[37] = 0xDEADBEEF;

  vane0.enable = 1;

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

logic<1> Pinwheel::take_branch(logic<32> insn, logic<32> reg_a, logic<32> reg_b) {
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

logic<32> Pinwheel::pc_gen(logic<32> pc, logic<32> insn, logic<1> active, logic<1> take_branch, logic<32> reg_a) {
  logic<5> op = b5(insn, 2);

  if (!active) {
    return pc;
  } else if (op == OP_BRANCH) {
    logic<32> imm_b = cat(dup<20>(insn[31]), insn[7], b6(insn, 25), b4(insn, 8), b1(0));
    return pc + (take_branch ? imm_b : b32(4));
  } else if (op == OP_JAL) {
    logic<32> imm_j = cat(dup<12>(insn[31]), b8(insn, 12), insn[20], b10(insn, 21), b1(0));
    return pc + imm_j;
  } else if (op == OP_JALR) {
    logic<32> imm_i = sign_extend<32>(b12(insn, 20));
    return reg_a + imm_i;
  } else {
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

  const auto vane0 = this->vane0;
  const auto vane1 = this->vane1;
  const auto vane2 = this->vane2;
  const auto temp_addr = this->temp_addr;
  const auto temp_alu  = this->temp_alu;

  const auto& data = this->data;
  const auto& code = this->code;
  const auto& regs = this->regs;

  logic<1>  take_branch = 0;
  logic<32> next_pc     = vane1.pc;

  logic<32> code_addr  = 0;
  logic<32> code_rdata = 0;

  logic<32> data_addr  = 0;
  logic<32> data_wdata = 0;
  logic<4>  data_wmask = 0;
  logic<32> data_rdata = 0;

  logic<10> reg_raddr1 = 0;
  logic<10> reg_raddr2 = 0;
  logic<10> reg_waddr  = 0;
  logic<1>  reg_wren   = 0;
  logic<32> reg_wdata  = 0;
  logic<32> alu_out    = 0;

  //----------

  logic<5> raddr_a = b5(vane1.insn, 15);
  logic<5> raddr_b = b5(vane1.insn, 20);

  logic<32> reg_a = raddr_a ? regs.out_a : b32(0);
  logic<32> reg_b = raddr_b ? regs.out_b : b32(0);

  if (reg_a == 0xCAFEBABE) {
    printf("???");
  }
  if (reg_b == 0xCAFEBABE) {
    printf("???");
  }

  //----------
  // Bus read mux

  switch(b4(temp_addr, 28)) {
    case 0x0: data_rdata = code.out;   break;
    case 0x1: data_rdata = regs.out_a; break;
    case 0x2: break;
    case 0x3: break;
    case 0x4: break;
    case 0x5: break;
    case 0x6: break;
    case 0x7: break;
    case 0x8: data_rdata = data.out;   break;
    case 0x9: break;
    case 0xA: break;
    case 0xB: break;
    case 0xC: break;
    case 0xD: break;
    case 0xE: break;
    case 0xF: data_rdata = debug_reg;  break;
  }

  //----------

  this->bus_to_reg = 0;
  this->reg_to_bus = 0;

  if (vane0.active) {
    logic<5> vane0_op = b5(vane0.insn, 2);

    reg_raddr1 = cat(vane0.hart, b5(code.out, 15));
    reg_raddr2 = cat(vane0.hart, b5(code.out, 20));
  }
  else if (vane1.active) {
    logic<5> op = b5(vane1.insn, 2);

    if (op == OP_LOAD) {
      auto addr  = addr_gen(vane1.insn, reg_a);

      if (b4(addr, 28) == 0x1) {
        this->reg_to_bus = true;
        reg_raddr1 = addr >> 2;
        reg_raddr2 = 0;
        //printf("hart %d reading regfile @ %d\n", (int)vane1.hart, (int)reg_raddr1);
      }
    }
  }

  if (vane1.active) {
    logic<5> op = b5(vane1.insn, 2);

    take_branch = this->take_branch(vane1.insn, reg_a, reg_b);

    if (op == OP_LOAD || op == OP_STORE) {
      auto addr  = addr_gen(vane1.insn, reg_a);

      //if (b4(addr, 28) != 0x8) printf("??? 0x%08x\n", (int)addr);

      if (b4(addr, 28) == 0x8) {
        data_addr   = addr;
        data_wdata  = reg_b;
        data_wmask  = mask_gen(vane1.insn, data_addr);
      }

      if (b4(addr, 28) == 0xF) {
        debug_reg = reg_b;
      }
    }

    alu_out     = alu(vane1.insn, vane1.pc, reg_a, reg_b);
    next_pc     = pc_gen(vane1.pc, vane1.insn, vane1.active, take_branch, reg_a);
  }

  if (vane2.enable | vane2.active) {
    logic<5> op = b5(vane2.insn, 2);

    if (op != OP_STORE && op != OP_BRANCH) {
      reg_waddr  = cat(vane2.hart, b5(vane2.insn, 7));
      reg_wren   = reg_waddr != 0 && op != OP_STORE && op != OP_BRANCH;
      reg_wdata  = op == OP_LOAD ? unpack(vane2.insn, temp_addr, data_rdata) : temp_alu;
    }

    code_addr  = vane2.pc;
  }
  else if (vane1.active) {
    logic<5> op = b5(vane1.insn, 2);

    if (op == OP_STORE) {
      auto addr  = addr_gen(vane1.insn, reg_a);

      if (b4(addr, 28) == 0x1) {
        this->bus_to_reg = 1;
        reg_waddr  = addr >> 2;
        reg_wren   = 1;
        reg_wdata  = reg_b;
        //printf("hart %d wrting regfile @ %d\n", (int)vane1.hart, (int)reg_waddr);
      }
    }

    code_addr = data_addr;
  }
  else {
    code_addr = data_addr;
  }

  //----------

  this->vane0 = vane2;
  this->vane0.active = vane2.enable | vane2.active;

  this->vane1 = vane0;

  if (vane0.active) {
    this->vane1.insn = code.out;
  }
  else {
    this->vane1.insn = 0;
  }

  this->vane2 = vane1;
  this->vane2.pc = next_pc;

  this->temp_addr = data_addr;
  this->temp_alu = alu_out;

  this->code.tick(code_addr, 0, 0);
  this->data.tick(data_addr, data_wdata, data_wmask);

  if (this->bus_to_reg && (reg_waddr < 32)) {
    printf("XXXXX\n");
  }

  if (this->reg_to_bus && (reg_raddr1 < 32)) {
    printf("%ld XXXXX\n", ticks);
  }

  this->regs.tick(reg_raddr1, reg_raddr2, reg_waddr, reg_wren, reg_wdata);
}

//--------------------------------------------------------------------------------
