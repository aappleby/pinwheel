#include "pinwheel.h"

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
  out_a = data[raddr1];
  out_b = data[raddr2];
}

void Regfile::tick_write(logic<10> waddr, logic<32> wdata, logic<1> wren) {
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

void Pinwheel::tick_fetch(logic<1> reset, logic<32> old_pc2, logic<32> old_insn1, logic<32> old_ra, logic<32> old_rb) {
  logic<5>  op  = b5(old_insn1, 2);
  logic<3>  f3  = b3(old_insn1, 12);
  logic<32> imm = decode_imm(old_insn1);

  logic<32> next_pc;
  if (reset || old_pc2 == 0) {
    next_pc = 0;
  }
  else {
    logic<1> eq  = old_ra == old_rb;
    logic<1> slt = signed(old_ra) < signed(old_rb);
    logic<1> ult = old_ra < old_rb;

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

    switch (op) {
      case OP_BRANCH:  next_pc = take_branch ? old_pc2 + imm : old_pc2 + b32(4); break;
      case OP_JAL:     next_pc = old_pc2 + imm; break;
      case OP_JALR:    next_pc = old_ra + imm; break;
      default:         next_pc = old_pc2 + 4; break;
    }
  }

  code.tick_read(next_pc);
  pc1 = next_pc;
}

//--------------------------------------------------------------------------------

void Pinwheel::tick_decode(logic<1> reset) {
  if (reset) {
    pc2 = 0x00400000 - 4;
    insn1 = 0;
  }
  else {
    pc2 = pc1;
    insn1 = pc1 == 0 ? b32(0) : code.out;
  }

  logic<5> next_ra = b5(code.out, 15);
  logic<5> next_rb = b5(code.out, 20);
  regfile.tick_read(b10(next_ra), b10(next_rb));

}

//--------------------------------------------------------------------------------

void Pinwheel::tick_execute(logic<1> reset) {
  if (reset) {
    insn2 = 0;
    result = 0;
    return;
  }

  logic<5>  op  = b5(insn1, 2);
  logic<3>  f3  = b3(insn1, 12);
  logic<7>  f7  = b7(insn1, 25);
  logic<32> imm = decode_imm(insn1);

  insn2 = insn1;

  switch(op) {
    case OP_JAL:     result = pc2 + 4;   return;
    case OP_JALR:    result = pc2 + 4;   return;
    case OP_LUI:     result = imm;       return;
    case OP_AUIPC:   result = pc2 + imm; return;
    case OP_LOAD:    result = regfile.out_a + imm; return;
    case OP_STORE:   result = regfile.out_a + imm; return;
  }

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

void Pinwheel::tick_memory(logic<1> reset) {
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

  data.tick_write(addr, regfile.out_b, mask, (op == OP_STORE) && data_cs);
  data.tick_read (addr);

  if (reset) {
    debug_reg = 0;
  }
  else {
    debug_reg = (op == OP_STORE) && debug_cs ? regfile.out_b : debug_reg;
  }

  if ((addr == 0x40000000) && (mask & 1) && (op == OP_STORE)) {
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
}

//----------

logic<32> Pinwheel::tock_memory() {
  logic<1> data_cs    = b4(result, 28) == 0x8;
  logic<1> debug_cs   = b4(result, 28) == 0xF;

  if (data_cs)  {
    return data.out;
  }
  else if (debug_cs) {
    return debug_reg;
  }
  else{
    return 0;
  }
}

//--------------------------------------------------------------------------------

void Pinwheel::tick_write(logic<1> reset) {
  logic<5>  op  = b5(insn2, 2);
  logic<5>  rd  = b5(insn2, 7);
  logic<3>  f3  = b3(insn2, 12);

  logic<32> data_out = tock_memory();

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

  auto writeback_addr = cat(b5(0), rd);
  auto writeback_wdata = op == OP_LOAD ? unpacked : result;
  auto writeback_wren = rd != 0 && op != OP_STORE && op != OP_BRANCH;

  regfile.tick_write(writeback_addr, writeback_wdata, writeback_wren);
}

//--------------------------------------------------------------------------------

void Pinwheel::tick_onecycle(logic<1> reset_in) {
  if (reset_in) {
    reset();
    ticks = 0;
    return;
  }

#if 0
  ticks++;

  code.tick_read(pc_old);

  logic<5>  op  = b5(code.out, 2);
  logic<5>  rd  = b5(code.out, 7);
  logic<3>  f3  = b3(code.out, 12);
  logic<5>  ra  = b5(code.out, 15);
  logic<5>  rb  = b5(code.out, 20);
  logic<7>  f7  = b7(code.out, 25);
  logic<32> imm = tock_imm(code.out);

  regfile.tick_read(b10(ra), b10(rb));

  auto bus_port  = get_bus(op, f3, imm, regfile.out_a, regfile.out_b);
  //tick_bus(bus_port);

  logic<1> data_cs    = b4(bus_port.addr, 28) == 0x8;
  logic<1> debug_cs   = b4(bus_port.addr, 28) == 0xF;

  data.tick_write(bus_port.addr, bus_port.wdata, bus_port.wmask, bus_port.wren && data_cs);
  data.tick_read (bus_port.addr);
  debug_reg = bus_port.wren && debug_cs ? bus_port.wdata : debug_reg;


  tock_bus(bus_port);

  auto alu_out   = tock_alu(op, f3, f7, imm, pc_old, regfile.out_a, regfile.out_b);
  auto unpacked  = tock_unpack(f3, bus_port.addr, bus_out);
  auto writeback = tock_wb(op, rd, unpacked, alu_out);

  regfile.tick_write(writeback.addr, writeback.wdata, writeback.wren);

  pc_old = tock_pc(reset_in, op, f3, imm, pc_old, regfile.out_a, regfile.out_b);
#endif
}

//--------------------------------------------------------------------------------

void Pinwheel::tick_twocycle(logic<1> reset_in) {
  if (reset_in) {
    reset();
  }

  logic<32> old_pc2   = pc2;
  logic<32> old_insn1 = insn1;
  logic<32> old_ra    = regfile.out_a;
  logic<32> old_rb    = regfile.out_b;

  tick_write  (reset_in);
  tick_memory (reset_in);
  tick_execute(reset_in);
  tick_decode (reset_in);
  tick_fetch  (reset_in, old_pc2, old_insn1, old_ra, old_rb);

  if (!reset_in) {
    ticks = ticks + 1;
  }
}

//--------------------------------------------------------------------------------
