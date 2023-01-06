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
  hart_b   = 0;
  hart_c = 0;
  hart_d = 0;

  pc_a = 0;
  pc_b = 0x00400000 - 4;

  insn_b = 0;
  insn_c = 0;

  result_c = 0;

  wb_addr_d = 0;
  wb_data_d = 0;
  wb_wren_d = 0;

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
    case RV32I_OP_LOAD:   return imm_i;
    case RV32I_OP_OPIMM:  return imm_i;
    case RV32I_OP_AUIPC:  return imm_u;
    case RV32I_OP_STORE:  return imm_s;
    case RV32I_OP_OP:     return imm_i;
    case RV32I_OP_LUI:    return imm_u;
    case RV32I_OP_BRANCH: return imm_b;
    case RV32I_OP_JALR:   return imm_i;
    case RV32I_OP_JAL:    return imm_j;
    default:              return 0;
  }
}

//--------------------------------------------------------------------------------

logic<32> Pinwheel::execute_alu(logic<32> insn, logic<32> reg_a, logic<32> reg_b) const {
  logic<5>  op  = b5(insn, 2);
  logic<3>  f3  = b3(insn, 12);
  logic<7>  f7  = b7(insn, 25);
  logic<32> imm = decode_imm(insn);

  logic<32> alu_a = reg_a;
  logic<32> alu_b = op == RV32I_OP_OPIMM ? imm : reg_b;
  if (op == RV32I_OP_OP && f3 == 0 && f7 == 32) alu_b = -alu_b;

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

  const auto hart_a = this->hart_a;
  const auto pc_a   = this->pc_a;
  const auto insn_a = code.out;
  const auto rs1_a  = b5(insn_a, 15);
  const auto rs2_a  = b5(insn_a, 20);

  const auto hart_b = this->hart_b;
  const auto pc_b   = this->pc_b;
  const auto insn_b = this->insn_b;
  const auto rs1_b  = regfile.out_rs1;
  const auto rs2_b  = regfile.out_rs2;
  const auto op_b   = b5(insn_b, 2);
  const auto f3_b   = b3(insn_b, 12);
  const auto f7_b   = b7(insn_b, 25);
  const auto imm_b  = decode_imm(insn_b);
  const auto addr_b = b32(rs1_b + imm_b);

  const bool data_wrcs_b    = b4(addr_b, 28) == 0x8;
  const bool debug_wrcs_b   = b4(addr_b, 28) == 0xF;
  const bool console_wrcs_b = b4(addr_b, 28) == 0x4;

  const auto hart_c   = this->hart_c;
  const auto insn_c   = this->insn_c;
  const auto result_c = this->result_c;
  const auto op_c     = b5(insn_c, 2);
  const auto rd_c     = b5(insn_c, 7);
  const auto f3_c     = b3(insn_c, 12);

  const auto hart_d    = this->hart_d;

  const auto debug_reg = this->debug_reg;

  //----------
  // Next PC

  logic<5>  next_hart_a = hart_b;
  logic<32> next_pc_a = 0;
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
      case RV32I_OP_BRANCH:  next_pc_a = take_branch ? pc_b + imm_b : pc_b + b32(4); break;
      case RV32I_OP_JAL:     next_pc_a = pc_b + imm_b; break;
      case RV32I_OP_JALR:    next_pc_a = addr_b; break;
      default:         next_pc_a = pc_b + 4; break;
    }
  }

  //----------
  // Write

  logic<1> data_rdcs_c    = b4(result_c, 28) == 0x8;
  logic<1> debug_rdcs_c   = b4(result_c, 28) == 0xF;

  logic<32> data_out_c = 0;
  if      (data_rdcs_c)  data_out_c = data.out;
  else if (debug_rdcs_c) data_out_c = debug_reg;

  if (result_c[0]) data_out_c = data_out_c >> 8;
  if (result_c[1]) data_out_c = data_out_c >> 16;

  logic<32> unpacked_c = data_out_c;

  switch (f3_c) {
    case 0:  unpacked_c = sign_extend<32>( b8(data_out_c)); break;
    case 1:  unpacked_c = sign_extend<32>(b16(data_out_c)); break;
    case 4:  unpacked_c = zero_extend<32>( b8(data_out_c)); break;
    case 5:  unpacked_c = zero_extend<32>(b16(data_out_c)); break;
  }


  self.hart_d    = hart_c;
  self.wb_addr_d = cat(b5(hart_c), rd_c);
  self.wb_data_d = op_c == RV32I_OP_LOAD ? unpacked_c : result_c;
  self.wb_wren_d = rd_c != 0 && op_c != RV32I_OP_STORE && op_c != RV32I_OP_BRANCH;


  if (op_c == RV32I_OP_CUSTOM0) {
    // Swap result and the PC that we'll use to fetch.
    // Execute phase should've deposited the new PC in result
    //printf("%08d> force_jump @ write from 0x%08x to 0x%08x\n", (int)ticks, (uint32_t)pc2, (uint32_t)old_result);
    self.wb_data_d = next_pc_a;
    next_pc_a      = result_c;
  }

  // This MUST come before self.regfile.tick_read.
  self.regfile.tick_write(wb_addr_d, wb_data_d, wb_wren_d);

  //----------
  // Execute

  logic<32> new_result;
  switch(op_b) {
    case RV32I_OP_JAL:     new_result = pc_b + 4;     break;
    case RV32I_OP_JALR:    new_result = pc_b + 4;     break;
    case RV32I_OP_LUI:     new_result = imm_b;        break;
    case RV32I_OP_AUIPC:   new_result = pc_b + imm_b; break;
    case RV32I_OP_LOAD:    new_result = addr_b;       break;
    case RV32I_OP_STORE:   new_result = addr_b;       break;
    case RV32I_OP_CUSTOM0: new_result = rs1_b;        break;
    case RV32I_OP_SYSTEM:  new_result = self.execute_system(insn_b); break;
    default:               new_result = self.execute_alu   (insn_b, rs1_b, rs2_b); break;
  }

  self.hart_c   = hart_b;
  self.insn_c   = insn_b;
  self.result_c = new_result;

  //----------
  // Fetch

  self.hart_a  = next_hart_a;
  self.pc_a    = next_pc_a;
  self.code.tick_read(next_pc_a);

  //----------
  // Memory

  logic<4>  mask_b  = 0;

  if (f3_b == 0) mask_b = 0b0001;
  if (f3_b == 1) mask_b = 0b0011;
  if (f3_b == 2) mask_b = 0b1111;

  if (addr_b[0]) mask_b = mask_b << 1;
  if (addr_b[1]) mask_b = mask_b << 2;

  self.data.tick_write(addr_b, rs2_b, mask_b, (op_b == RV32I_OP_STORE) && data_wrcs_b);
  self.data.tick_read (addr_b);

  //----------
  // Debug reg

  self.debug_reg = (op_b == RV32I_OP_STORE) && debug_wrcs_b ? rs2_b : debug_reg;

  if (console_wrcs_b && op_b == RV32I_OP_STORE) {
    self.tick_console(rs2_b);
  }

  //----------
  // Decode

  self.hart_b  = hart_a;
  self.pc_b    = pc_a;
  self.insn_b  = pc_a == 0 ? b32(0) : insn_a;
  self.regfile.tick_read(cat(b5(hart_a), rs1_a), cat(b5(hart_a), rs2_a));

  //----------

  if (reset_in) {
    self.reset();
  }
  else {
    self.ticks = ticks + 1;
  }
}

//--------------------------------------------------------------------------------
