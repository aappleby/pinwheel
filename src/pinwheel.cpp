#include "pinwheel.h"

#include <assert.h>

//--------------------------------------------------------------------------------

void BlockRam::tock(logic<32> addr, logic<32> wdata, logic<4> wmask, logic<1> wren) {
  this->addr  = addr;
  this->wdata = wdata;
  this->wmask = wmask;
  this->wren  = wren;
}

void BlockRam::tick() {
  if (wren) {
    logic<32> old_data = data[b10(addr, 2)];
    logic<32> new_data = wdata;
    if (addr[0]) new_data = new_data << 8;
    if (addr[1]) new_data = new_data << 16;

    data[b10(addr, 2)] = ((wmask[0] ? new_data : old_data) & 0x000000FF) |
                         ((wmask[1] ? new_data : old_data) & 0x0000FF00) |
                         ((wmask[2] ? new_data : old_data) & 0x00FF0000) |
                         ((wmask[3] ? new_data : old_data) & 0xFF000000);
  }
  out = data[b10(addr, 2)];
}

//--------------------------------------------------------------------------------

void Regfile::tock(logic<10> raddr1, logic<10> raddr2, logic<10> waddr, logic<32> wdata, logic<1> wren) {

  /*
  if (raddr1 == 0) {
    printf("?");
  }
  if (raddr2 == 0) {
    printf("?");
  }
  */

  assert(raddr1 < 1024);
  assert(raddr2 < 1024);
  assert(waddr < 1024);
  this->raddr1 = raddr1;
  this->raddr2 = raddr2;
  this->waddr = waddr;
  this->wdata = wdata;
  this->wren  = wren;
}

void Regfile::tick() {
  if (wren) data[waddr] = wdata;
  out_rs1 = data[raddr1];
  out_rs2 = data[raddr2];
}

//--------------------------------------------------------------------------------

Pinwheel* Pinwheel::clone() {
  Pinwheel* p = new Pinwheel();
  memcpy(p, this, sizeof(*this));
  return p;
}

//--------------------------------------------------------------------------------

void Pinwheel::reset_mem() {
  memset(&code,    0x00, sizeof(code));
  memset(&data,    0x00, sizeof(data));
  memset(&regfile, 0, sizeof(regfile));

  std::string s;
  value_plusargs("text_file=%s", s);
  readmemh(s, code.data);

  value_plusargs("data_file=%s", s);
  readmemh(s, data.data);
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

void Console::tock(logic<1> wrcs, logic<32> reg_b) {
  this->wrcs = wrcs;
  this->reg_b = reg_b;
}

void Console::tick(logic<1> reset) {
  if (reset) {
    memset(buf, 0, sizeof(buf));
    x = 0;
    y = 0;
  }
  else if (wrcs) {
    buf[y * 80 + x] = 0;
    auto c = char(reg_b);

    if (c == 0) c = '?';

    if (c == '\n') {
      x = 0;
      y++;
    }
    else if (c == '\r') {
      x = 0;
    }
    else {
      buf[y * 80 + x] = c;
      x++;
    }

    if (x == 80) {
      x = 0;
      y++;
    }
    if (y == 25) {
      memcpy(buf, buf + 80, 80*24);
      memset(buf + (80*24), 0, 80);
      y = 24;
    }
    buf[y * 80 + x] = 30;
  }
}

//--------------------------------------------------------------------------------

void Pinwheel::tock_twocycle(logic<1> reset_in) const {
  Pinwheel& self = const_cast<Pinwheel&>(*this);

  const auto op_b   = b5(insn_b, 2);
  const auto rda_b  = b5(insn_b, 7);
  const auto f3_b   = b3(insn_b, 12);
  const auto rs1a_b = b5(insn_b, 15);
  const auto rs2a_b = b5(insn_b, 20);
  const auto f7_b   = b7(insn_b, 25);

  const auto rs1_b  = rs1a_b ? regfile.out_rs1 : b32(0);
  const auto rs2_b  = rs2a_b ? regfile.out_rs2 : b32(0);
  const auto imm_b  = decode_imm(insn_b);
  const auto addr_b = b32(rs1_b + imm_b);
  //const auto addr_c = result_c;

  //----------
  // Fetch

  self.next_hart_a = hart_b;
  self.next_pc_a = 0;

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
        case RV32I_OP_BRANCH:  self.next_pc_a = take_branch ? pc_b + imm_b : pc_b + b32(4); break;
        case RV32I_OP_JAL:     self.next_pc_a = pc_b + imm_b; break;
        case RV32I_OP_JALR:    self.next_pc_a = addr_b; break;
        default:               self.next_pc_a = pc_b + 4; break;
      }
    }
  }

  //----------
  // Decode

  const auto insn_a = code.out;
  const auto rs1a_a  = b5(insn_a, 15);
  const auto rs2a_a  = b5(insn_a, 20);

  self.next_insn_b = pc_a == 0 ? b32(0) : insn_a;
  self.next_addr_c = addr_b;

  //----------
  // Execute

  switch(op_b) {
    case RV32I_OP_JAL:     self.next_result_c = pc_b + 4;     break;
    case RV32I_OP_JALR:    self.next_result_c = pc_b + 4;     break;
    case RV32I_OP_LUI:     self.next_result_c = imm_b;        break;
    case RV32I_OP_AUIPC:   self.next_result_c = pc_b + imm_b; break;
    case RV32I_OP_LOAD:    self.next_result_c = addr_b;       break;
    case RV32I_OP_STORE:   self.next_result_c = rs2_b;        break;
    case RV32I_OP_CUSTOM0: {
      if (f3_b == 0) {
        // Switch the other thread to another hart
        self.next_addr_c   = rs1_b;
        self.next_result_c = rs2_b;
      }
      else if (f3_b == 1) {
        // Yield to another hart
        self.next_result_c  = self.next_pc_a;
        self.next_hart_a    = rs1_b;
        self.next_pc_a      = rs2_b;
      }
      break;
    }
    case RV32I_OP_SYSTEM:  self.next_result_c = execute_system(insn_b); break;
    default:               self.next_result_c = execute_alu   (insn_b, rs1_b, rs2_b); break;
  }


  //----------
  // Memory

  auto console1_cs_b = b4(addr_b, 28) == 0x4;
  auto console2_cs_b = b4(addr_b, 28) == 0x5;
  auto data_cs_b    = b4(addr_b, 28) == 0x8;
  auto regfile_cs_b = b4(addr_b, 28) == 0xE;
  auto debug_cs_b   = b4(addr_b, 28) == 0xF;

  logic<4> next_mask_b = 0;
  if (f3_b == 0) next_mask_b = 0b0001;
  if (f3_b == 1) next_mask_b = 0b0011;
  if (f3_b == 2) next_mask_b = 0b1111;
  if (addr_b[0]) next_mask_b = next_mask_b << 1;
  if (addr_b[1]) next_mask_b = next_mask_b << 2;

  self.next_debug_reg = (op_b == RV32I_OP_STORE) && debug_cs_b ? rs2_b : debug_reg;

  //----------
  // Write

  const auto op_c     = b5(insn_c, 2);
  const auto rd_c     = b5(insn_c, 7);
  const auto f3_c     = b3(insn_c, 12);

  auto console_cs_c = b4(addr_c, 28) == 0x4;
  auto data_cs_c    = b4(addr_c, 28) == 0x8;
  auto debug_cs_c   = b4(addr_c, 28) == 0xF;
  auto regfile_cs_c = b4(addr_c, 28) == 0xE;

  logic<32> data_out_c = 0;
  if      (data_cs_c)    data_out_c = data.out;
  else if (debug_cs_c)   data_out_c = debug_reg;
  else if (regfile_cs_c) data_out_c = regfile.out_rs1;

  logic<32> unpacked_c = data_out_c;
  if (result_c[0]) unpacked_c = unpacked_c >> 8;
  if (result_c[1]) unpacked_c = unpacked_c >> 16;
  switch (f3_c) {
    case 0:  unpacked_c = sign_extend<32>( b8(unpacked_c)); break;
    case 1:  unpacked_c = sign_extend<32>(b16(unpacked_c)); break;
    case 4:  unpacked_c = zero_extend<32>( b8(unpacked_c)); break;
    case 5:  unpacked_c = zero_extend<32>(b16(unpacked_c)); break;
  }

  self.next_wb_addr_d = cat(b5(hart_c), rd_c);
  self.next_wb_data_d = op_c == RV32I_OP_LOAD ? unpacked_c : result_c;
  self.next_wb_wren_d = /*rd_c != 0 &&*/ op_c != RV32I_OP_STORE && op_c != RV32I_OP_BRANCH;

  if (op_c == RV32I_OP_CUSTOM0 && f3_c == 0) {
    // Swap result and the PC that we'll use to fetch.
    // Execute phase should've deposited the new PC in result
    //printf("%08d> force_jump @ write from 0x%08x to 0x%08x\n", (int)ticks, (uint32_t)pc2, (uint32_t)old_result);
    self.next_wb_data_d = next_pc_a;
    self.next_hart_a    = addr_c;
    self.next_pc_a      = result_c;
  }

  if (regfile_cs_c && op_c == RV32I_OP_STORE) {
    // Thread writing to other thread's regfile
    self.next_wb_addr_d = b10(addr_c >> 2);
    self.next_wb_data_d = result_c;
    self.next_wb_wren_d = 1;
  }

  //----------

  const bool data_wren_b = (op_b == RV32I_OP_STORE) && data_cs_b;
  const bool regfile_wren_b = (op_b == RV32I_OP_STORE) && regfile_cs_b;

  auto reg_raddr1_a = cat(b5(hart_a), rs1a_a);
  auto reg_raddr2_a = cat(b5(hart_a), rs2a_a);

  if ((op_b == RV32I_OP_LOAD) && regfile_cs_b && (pc_a == 0)) {
    reg_raddr1_a = b10(addr_b >> 2);
  }

  self.code.tock(next_pc_a, 0, 0, 0);
  self.data.tock(addr_b, rs2_b, next_mask_b, data_wren_b);
  self.regfile.tock(reg_raddr1_a, reg_raddr2_a, next_wb_addr_d, next_wb_data_d, next_wb_wren_d);
  self.console1.tock(console1_cs_b && op_b == RV32I_OP_STORE, rs2_b);
  self.console2.tock(console2_cs_b && op_b == RV32I_OP_STORE, rs2_b);
}

//--------------------------------------------------------------------------------

void Pinwheel::tick_twocycle(logic<1> reset_in) const {
  Pinwheel& self = const_cast<Pinwheel&>(*this);

  if (reset_in) {
    self.reset_mem();
    self.hart_a    = 1;
    self.pc_a      = 0;

    self.hart_b    = 0;
    self.pc_b      = 0x00400000 - 4;
    self.insn_b    = 0;

    self.hart_c    = 0;
    self.pc_c      = 0;
    self.insn_c    = 0;
    self.addr_c    = 0;
    self.result_c  = 0;

    self.hart_d    = 0;
    self.pc_d      = 0;
    self.insn_d    = 0;
    self.result_d  = 0;
    self.wb_addr_d = 0;
    self.wb_data_d = 0;
    self.wb_wren_d = 0;

    self.debug_reg = 0;
    self.ticks     = 0;
  }
  else {
    self.hart_d    = hart_c;
    self.pc_d      = pc_c;
    self.insn_d    = insn_c;
    self.result_d  = result_c;
    self.wb_addr_d = next_wb_addr_d;
    self.wb_data_d = next_wb_data_d;
    self.wb_wren_d = next_wb_wren_d;

    self.hart_c    = hart_b;
    self.pc_c      = pc_b;
    self.insn_c    = insn_b;
    self.addr_c    = next_addr_c;
    self.result_c  = next_result_c;

    self.hart_b    = hart_a;
    self.pc_b      = pc_a;
    self.insn_b    = next_insn_b;

    self.hart_a    = next_hart_a;
    self.pc_a      = next_pc_a;

    self.debug_reg = next_debug_reg;
    self.ticks     = ticks + 1;
  }

  self.code.tick();
  self.data.tick();
  self.regfile.tick();
  self.console1.tick(reset_in);
  self.console2.tick(reset_in);
}

//--------------------------------------------------------------------------------
