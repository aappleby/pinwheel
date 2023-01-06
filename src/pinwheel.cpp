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

  out_a = data[raddr1];
  out_b = data[raddr2];
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

  hart1 = 1;
  //pc1 = 0x00400000 - 4;
  pc1 = 0;

  hart2 = 0;
  pc2 = 0x00400000 - 4;

  insn1 = 0;
  insn2 = 0;
  result = 0;

  writeback_addr = 0;
  writeback_data = 0;
  writeback_wren = 0;

  debug_reg = 0;

  force_jump = 0;
  jump_dest = 0;

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

void Pinwheel::execute_alu() {
  logic<5>  op  = b5(insn1, 2);
  logic<3>  f3  = b3(insn1, 12);
  logic<7>  f7  = b7(insn1, 25);
  logic<32> imm = decode_imm(insn1);

  logic<32> alu_a = regfile.out_a;
  logic<32> alu_b = op == OP_ALUI ? imm : regfile.out_b;
  if (op == OP_ALU && f3 == 0 && f7 == 32) alu_b = -alu_b;

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
}

//--------------------------------------------------------------------------------

void Pinwheel::execute_custom() {
  //printf("custom op! 0x%08x\n", (uint32_t)regfile.out_a);
  // This doesn't work quite right.
  // Probably need to stash it in a register until the other thread hits fetch state

  result = pc1;
  //pc1 = regfile.out_a;

  printf("%08d> force_jump @ execute from 0x%08x to 0x%08x\n", (int)ticks, (uint32_t)pc1, (uint32_t)regfile.out_a);
  force_jump = 1;
  jump_dest = regfile.out_a;
}

//--------------------------------------------------------------------------------

void Pinwheel::execute_system() {
  logic<3>  f3  = b3(insn1, 12);
  auto csr = b12(insn1, 20);
  result = 0;

  switch(f3) {
    case 0:               break;
    case RV32I_F3_CSRRW:  break;
    case RV32I_F3_CSRRS:  if (csr == 0xF14) result = hart2; break;
    case RV32I_F3_CSRRC:  break;
    case 4:               break;
    case RV32I_F3_CSRRWI: break;
    case RV32I_F3_CSRRSI: break;
    case RV32I_F3_CSRRCI: break;
  }
}

//--------------------------------------------------------------------------------

void Pinwheel::tick_console() {
  console_buf[console_y * 80 + console_x] = 0;
  auto c = char(regfile.out_b);

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

void Pinwheel::tick_twocycle(logic<1> reset_in) {
  // FIXME the resetting of registers while still sending addresses to buses is still broken

  if (reset_in) {
    force_jump = 0;
    jump_dest = 0;
  }

  auto old_hart1  = hart1;
  auto old_hart2  = hart2;
  auto old_pc1    = pc1;
  auto old_pc2    = pc2;
  auto old_insn1  = insn1;
  auto old_insn2  = insn2;
  auto old_result = result;

  auto old_writeback_addr = writeback_addr;
  auto old_writeback_data = writeback_data;
  auto old_writeback_wren = writeback_wren;

  auto old_debug_reg = debug_reg;
  auto old_force_jump = force_jump;
  auto old_jump_dest = jump_dest;

  auto old_ra     = regfile.out_a;
  auto old_rb     = regfile.out_b;

  //----------
  // Write

  logic<5>  op  = b5(insn2, 2);
  logic<5>  rd  = b5(insn2, 7);
  logic<3>  f3  = b3(insn2, 12);

  logic<1> data_cs    = b4(result, 28) == 0x8;
  logic<1> debug_cs   = b4(result, 28) == 0xF;

  logic<32> data_out = 0;
  if      (data_cs)  data_out = data.out;
  else if (debug_cs) data_out = debug_reg;

  if (result[0]) data_out = data_out >> 8;
  if (result[1]) data_out = data_out >> 16;

  logic<32> unpacked;
  switch (f3) {
    case 0:  unpacked = sign_extend<32>( b8(data_out)); break;
    case 1:  unpacked = sign_extend<32>(b16(data_out)); break;
    case 2:  unpacked = data_out; break;
    case 3:  unpacked = data_out; break;
    case 4:  unpacked = zero_extend<32>( b8(data_out)); break;
    case 5:  unpacked = zero_extend<32>(b16(data_out)); break;
    case 6:  unpacked = data_out; break;
    case 7:  unpacked = data_out; break;
    default: unpacked = 0; break;
  }

  writeback_addr = cat(b5(hart1), rd);
  writeback_data = op == OP_LOAD ? unpacked : result;
  writeback_wren = rd != 0 && op != OP_STORE && op != OP_BRANCH;
  regfile.tick_write(writeback_addr, writeback_data, writeback_wren);

  //----------
  // Memory

  {
    logic<5>  op  = b5(insn1, 2);
    logic<3>  f3  = b3(insn1, 12);
    logic<32> imm = decode_imm(insn1);

    logic<32> addr  = regfile.out_a + imm;
    logic<4>  mask  = 0;

    if (f3 == 0) mask = 0b0001;
    if (f3 == 1) mask = 0b0011;
    if (f3 == 2) mask = 0b1111;

    if (addr[0]) mask = mask << 1;
    if (addr[1]) mask = mask << 2;

    logic<1> data_cs    = b4(addr, 28) == 0x8;
    logic<1> debug_cs   = b4(addr, 28) == 0xF;
    logic<1> console_cs = b4(addr, 28) == 0x4;

    data.tick_write(addr, regfile.out_b, mask, (op == OP_STORE) && data_cs);
    data.tick_read (addr);

    debug_reg = (op == OP_STORE) && debug_cs ? regfile.out_b : debug_reg;

    if (console_cs && op == OP_STORE) {
      tick_console();
    }
  }

  //----------
  // Execute

  {
    logic<5>  op  = b5(insn1, 2);
    logic<3>  f3  = b3(insn1, 12);
    logic<7>  f7  = b7(insn1, 25);
    logic<32> imm = decode_imm(insn1);

    switch(op) {
      case OP_JAL:           result = pc2 + 4;   break;
      case OP_JALR:          result = pc2 + 4;   break;
      case OP_LUI:           result = imm;       break;
      case OP_AUIPC:         result = pc2 + imm; break;
      case OP_LOAD:          result = regfile.out_a + imm; break;
      case OP_STORE:         result = regfile.out_a + imm; break;
      case RV32I_OP_CUSTOM0: execute_custom(); break;
      case OP_SYSTEM:        execute_system(); break;
      default:               execute_alu(); break;
    }
  }


  insn2 = insn1;

  //----------
  // Decode

  /*
  if (force_jump) {
    printf("%08d> force_jump @ fetch from 0x%08x to 0x%08x\n", (int)ticks, (uint32_t)pc1, (uint32_t)jump_dest);
  }
  */

  logic<5> next_ra = b5(code.out, 15);
  logic<5> next_rb = b5(code.out, 20);
  regfile.tick_read(cat(b5(hart1), next_ra), cat(b5(hart1), next_rb));

  pc2 = old_pc1;
  hart2 = old_hart1;
  insn1 = old_pc1 == 0 ? b32(0) : code.out;

  //----------

  {
    logic<5>  old_op1  = b5(old_insn1, 2);
    logic<3>  old_f31  = b3(old_insn1, 12);
    logic<32> old_imm1 = decode_imm(old_insn1);

    logic<32> new_pc1;
    if (old_pc2 == 0) {
      new_pc1 = 0;
    }
    else {
      logic<1> eq  = old_ra == old_rb;
      logic<1> slt = signed(old_ra) < signed(old_rb);
      logic<1> ult = old_ra < old_rb;

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
        case OP_JALR:    new_pc1 = old_ra + old_imm1; break;
        default:         new_pc1 = old_pc2 + 4; break;
      }
    }

    pc1 = new_pc1;
    hart1 = old_hart2;

    code.tick_read(new_pc1);
  }

  //----------

  if (reset_in) {
    reset();
  }
  else {
    ticks = ticks + 1;
  }
}

//--------------------------------------------------------------------------------
