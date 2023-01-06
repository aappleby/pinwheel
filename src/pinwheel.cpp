#include "pinwheel.h"

#include <assert.h>

//--------------------------------------------------------------------------------

void BlockRam::tick_read(logic<32> raddr) {
  out = data[b10(raddr, 2)];
}

void BlockRam::tick_write(logic<32> waddr, logic<32> wdata, logic<4> wmask, logic<1> wren) {
  if (wren) {
    logic<32> old_data = data[b10(waddr, 2)];
    logic<32> new_data = wdata;
    if (waddr[0]) new_data = new_data << 8;
    if (waddr[1]) new_data = new_data << 16;

    data[b10(waddr, 2)] = ((wmask[0] ? new_data : old_data) & 0x000000FF) |
                          ((wmask[1] ? new_data : old_data) & 0x0000FF00) |
                          ((wmask[2] ? new_data : old_data) & 0x00FF0000) |
                          ((wmask[3] ? new_data : old_data) & 0xFF000000);
  }
}

//--------------------------------------------------------------------------------

void Regfile::tick_read(logic<10> raddr1, logic<10> raddr2) {
  assert(raddr1 < 1024);
  assert(raddr2 < 1024);

  out_rs1 = data[raddr1];
  out_rs2 = data[raddr2];
}

void Regfile::tick_write(logic<10> waddr, logic<32> wdata, logic<1> wren) {
  assert(waddr < 1024);

  if (wren) {
    data[waddr] = wdata;
  }
}

//--------------------------------------------------------------------------------

Pinwheel* Pinwheel::clone() {
  Pinwheel* p = new Pinwheel();
  memcpy(p, this, sizeof(*this));
  return p;
}

//--------------------------------------------------------------------------------

void Pinwheel::reset() {
  debug_reg = 0;

  hart_a = 1;
  pc_a   = 0;
  insn_a = 0;
  result_a = 0;

  hart_b   = 0;
  pc_b     = 0x00400000 - 4;
  insn_b   = 0;

  writeback_addr = 0;
  writeback_data = 0;
  writeback_wren = 0;

  debug_reg = 0;

  memset(&code, 0, sizeof(code));
  memset(&data, 0, sizeof(data));
  memset(&regfile, 0, sizeof(regfile));

  std::string s;
  value_plusargs("text_file=%s", s);
  readmemh(s, code.data);

  value_plusargs("data_file=%s", s);
  readmemh(s, data.data);

  memset(console_buf, 0, sizeof(console_buf));
  console_x = 0;
  console_y = 0;
  ticks = 0;
}

//--------------------------------------------------------------------------------

logic<32> Pinwheel::decode_imm(logic<32> insn) {
  logic<5>  op    = b5(insn, 2);
  logic<32> imm_i = sign_extend<32>(b12(insn, 20));
  logic<32> imm_s = cat(dup<21>(insn[31]), b6(insn, 25), b5(insn, 7));
  logic<32> imm_u = b20(insn, 12) << 12;
  logic<32> imm_b = cat(dup<20>(insn[31]), insn[7], b6(insn, 25), b4(insn, 8), b1(0));
  logic<32> imm_j = cat(dup<12>(insn[31]), b8(insn, 12), insn[20], b10(insn, 21), b1(0));

  switch(op) {
    case OP_LOAD:   return imm_i;
    case OP_ALUI:   return imm_i;
    case OP_AUIPC:  return imm_u;
    case OP_STORE:  return imm_s;
    case OP_ALU:    return imm_i;
    case OP_LUI:    return imm_u;
    case OP_BRANCH: return imm_b;
    case OP_JALR:   return imm_i;
    case OP_JAL:    return imm_j;
    default:        return 0;
  }
}

//--------------------------------------------------------------------------------

logic<32> Pinwheel::execute_alu(logic<32> insn, logic<32> reg_a, logic<32> reg_b) const {
  logic<5>  op  = b5(insn, 2);
  logic<3>  f3  = b3(insn, 12);
  logic<7>  f7  = b7(insn, 25);
  logic<32> imm = decode_imm(insn);

  logic<32> alu_a = reg_a;
  logic<32> alu_b = op == OP_ALUI ? imm : reg_b;
  if (op == OP_ALU && f3 == 0 && f7 == 32) alu_b = -alu_b;

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

//--------------------------------------------------------------------------------

logic<32> Pinwheel::execute_custom(logic<32> insn, logic<32> reg_a) {
  //printf("custom op! 0x%08x\n", (uint32_t)regfile.out_a);

  return reg_a;
}

//--------------------------------------------------------------------------------

logic<32> Pinwheel::execute_system(logic<32> insn) const {
  logic<3>  f3  = b3(insn, 12);
  auto csr = b12(insn, 20);

  logic<32> result = 0;
  switch(f3) {
    case 0:               break;
    case RV32I_F3_CSRRW:  break;
    case RV32I_F3_CSRRS:  if (csr == 0xF14) result = hart_b; break;
    case RV32I_F3_CSRRC:  break;
    case 4:               break;
    case RV32I_F3_CSRRWI: break;
    case RV32I_F3_CSRRSI: break;
    case RV32I_F3_CSRRCI: break;
  }
  return result;
}

//--------------------------------------------------------------------------------

void Pinwheel::tick_console(logic<32> reg_b) {
  console_buf[console_y * 80 + console_x] = 0;
  auto c = char(reg_b);

  if (c == 0) c = '?';

  if (c == '\n') {
    console_x = 0;
    console_y++;
  }
  else if (c == '\r') {
    console_x = 0;
  }
  else {
    console_buf[console_y * 80 + console_x] = c;
    console_x++;
  }

  if (console_x == 80) {
    console_x = 0;
    console_y++;
  }
  if (console_y == 25) {
    memcpy(console_buf, console_buf + 80, 80*24);
    memset(console_buf + (80*24), 0, 80);
    console_y = 24;
  }
  console_buf[console_y * 80 + console_x] = 30;
}

//--------------------------------------------------------------------------------
// FIXME the resetting of registers while still sending addresses to buses is still broken

void Pinwheel::tick_twocycle(logic<1> reset_in) const {
  Pinwheel& self = const_cast<Pinwheel&>(*this);

  if (reset_in) {
  }

  auto old_hart1  = hart_a;
  auto old_hart2  = hart_b;
  auto old_pc1    = pc_a;
  auto old_pc2    = pc_b;
  auto old_insn1  = insn_b;
  auto old_insn2  = insn_a;
  auto old_result = result_a;

  auto old_debug_reg = debug_reg;

  auto old_code_out = code.out;
  auto next_ra = b5(old_code_out, 15);
  auto next_rb = b5(old_code_out, 20);

  auto old_reg_a = regfile.out_rs1;
  auto old_reg_b = regfile.out_rs2;

  auto old_op1  = b5(old_insn1, 2);
  auto old_f31  = b3(old_insn1, 12);
  auto old_f71  = b7(old_insn1, 25);
  auto old_imm1 = decode_imm(old_insn1);

  auto old_op2  = b5(old_insn2, 2);
  auto old_rd2  = b5(old_insn2, 7);
  auto old_f32  = b3(old_insn2, 12);

  logic<32> old_addr = old_reg_a + old_imm1;

  logic<1> old_data_wrcs    = b4(old_addr, 28) == 0x8;
  logic<1> old_debug_wrcs   = b4(old_addr, 28) == 0xF;
  logic<1> old_console_wrcs = b4(old_addr, 28) == 0x4;

  //----------
  // Next PC

  logic<32> new_pc1 = 0;
  if (old_pc2) {
    logic<1> eq  = old_reg_a == old_reg_b;
    logic<1> slt = signed(old_reg_a) < signed(old_reg_b);
    logic<1> ult = old_reg_a < old_reg_b;

    logic<1> take_branch;
    switch (old_f31) {
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

    switch (old_op1) {
      case OP_BRANCH:  new_pc1 = take_branch ? old_pc2 + old_imm1 : old_pc2 + b32(4); break;
      case OP_JAL:     new_pc1 = old_pc2 + old_imm1; break;
      case OP_JALR:    new_pc1 = old_addr; break;
      default:         new_pc1 = old_pc2 + 4; break;
    }
  }

  //----------
  // Write

  logic<1> old_data_rdcs    = b4(old_result, 28) == 0x8;
  logic<1> old_debug_rdcs   = b4(old_result, 28) == 0xF;

  logic<32> old_data_out = 0;
  if      (old_data_rdcs)  old_data_out = data.out;
  else if (old_debug_rdcs) old_data_out = debug_reg;

  if (old_result[0]) old_data_out = old_data_out >> 8;
  if (old_result[1]) old_data_out = old_data_out >> 16;

  logic<32> unpacked = old_data_out;

  switch (old_f32) {
    case 0:  unpacked = sign_extend<32>( b8(old_data_out)); break;
    case 1:  unpacked = sign_extend<32>(b16(old_data_out)); break;
    case 4:  unpacked = zero_extend<32>( b8(old_data_out)); break;
    case 5:  unpacked = zero_extend<32>(b16(old_data_out)); break;
  }

  self.writeback_addr = cat(b5(hart_a), old_rd2);
  self.writeback_data = old_op2 == OP_LOAD ? unpacked : old_result;
  self.writeback_wren = old_rd2 != 0 && old_op2 != OP_STORE && old_op2 != OP_BRANCH;

  if (old_op2 == RV32I_OP_CUSTOM0) {
    // Swap result and the PC that we'll use to fetch.
    // Execute phase should've deposited the new PC in result
    //printf("%08d> force_jump @ write from 0x%08x to 0x%08x\n", (int)ticks, (uint32_t)pc2, (uint32_t)old_result);
    self.writeback_data = new_pc1;
    new_pc1 = old_result;
  }

  // This MUST come before self.regfile.tick_read.
  self.regfile.tick_write(writeback_addr, writeback_data, writeback_wren);

  //----------
  // Execute

  logic<32> new_result;
  switch(old_op1) {
    case OP_JAL:           new_result = pc_b + 4;   break;
    case OP_JALR:          new_result = pc_b + 4;   break;
    case OP_LUI:           new_result = old_imm1;       break;
    case OP_AUIPC:         new_result = pc_b + old_imm1; break;
    case OP_LOAD:          new_result = old_addr; break;
    case OP_STORE:         new_result = old_addr; break;
    case RV32I_OP_CUSTOM0: {
      //printf("%08d> force_jump @ execute from 0x%08x to 0x%08x\n", (int)ticks, (uint32_t)pc1, (uint32_t)old_reg_a);
      new_result = old_reg_a;
      break;
    }
    case OP_SYSTEM:        new_result = self.execute_system(old_insn1); break;
    default:               new_result = self.execute_alu   (old_insn1, old_reg_a, old_reg_b); break;
  }

  self.insn_a  = old_insn1;
  self.result_a = new_result;

  //----------
  // Fetch

  self.pc_a    = new_pc1;
  self.code.tick_read(new_pc1);

  //----------
  // Memory

  logic<4>  mask  = 0;

  if (old_f31 == 0) mask = 0b0001;
  if (old_f31 == 1) mask = 0b0011;
  if (old_f31 == 2) mask = 0b1111;

  if (old_addr[0]) mask = mask << 1;
  if (old_addr[1]) mask = mask << 2;

  self.data.tick_write(old_addr, old_reg_b, mask, (old_op1 == OP_STORE) && old_data_wrcs);
  self.data.tick_read (old_addr);

  //----------
  // Debug reg

  self.debug_reg = (old_op1 == OP_STORE) && old_debug_wrcs ? old_reg_b : old_debug_reg;

  if (old_console_wrcs && old_op1 == OP_STORE) {
    self.tick_console(old_reg_b);
  }

  //----------
  // Decode

  self.pc_b    = old_pc1;
  self.insn_b  = old_pc1 == 0 ? b32(0) : old_code_out;
  self.regfile.tick_read(cat(b5(old_hart1), next_ra), cat(b5(old_hart1), next_rb));

  //----------

  self.hart_a  = old_hart2;
  self.hart_b  = old_hart1;

  //----------

  if (reset_in) {
    self.reset();
  }
  else {
    self.ticks = ticks + 1;
  }
}

//--------------------------------------------------------------------------------
