#pragma once
#include "metron_tools.h"
#include "constants.h"

#define wb =

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

struct cpu_to_mem {
  logic<32> addr;
  logic<32> data;
  logic<4>  mask;
};

struct rbus_read {
  logic<10> raddr1;
  logic<10> raddr2;
};

struct rbus_write {
  logic<10> waddr;
  logic<32> wdata;
  logic<1>  wren;
};

struct registers_p0 {
  logic<5>  hart;
  logic<32> pc;
  logic<1>  enable;
  logic<1>  active;
};

struct registers_p1 {
  logic<5>  hart;
  logic<32> pc;
  logic<32> insn;
  logic<1>  enable;
  logic<1>  active;
};

struct registers_p2 {
  logic<5>  hart;
  logic<32> pc;
  logic<32> insn;

  logic<5>  align;
  logic<32> alu_out;
  logic<1>  enable;
  logic<1>  active;
};

//------------------------------------------------------------------------------

class Pinwheel {
 public:
  void tock(logic<1> reset) {
    tick(reset);
  }

  uint32_t code_mem[16384];  // Cores share ROM
  uint32_t data_mem[16384];  // Cores share RAM
  uint32_t regfile[1024];    // Cores have their own register files

  registers_p0 reg_p0;
  registers_p1 reg_p1;
  registers_p2 reg_p2;

  logic<32> ra;
  logic<32> rb;
  logic<32> dbus_data;
  logic<32> pbus_data;

  //----------------------------------------

  void reset() {
    printf("pinwheel::reset()\n");
    memset(code_mem, 0, sizeof(code_mem));
    memset(data_mem, 0, sizeof(data_mem));

    std::string s;
    value_plusargs("text_file=%s", s);
    readmemh(s, code_mem);

    value_plusargs("data_file=%s", s);
    readmemh(s, data_mem);

    memset(regfile, 0, sizeof(regfile));

    reg_p0.hart   = 0;
    reg_p0.pc     = 0;
    reg_p0.enable = 1;
    reg_p0.active = 0;

    reg_p1.hart   = 1;
    reg_p1.pc     = 0;
    reg_p1.insn   = 0;
    reg_p1.enable = 0;
    reg_p1.active = 0;

    reg_p2.hart   = 2;
    reg_p2.pc     = 0;
    reg_p2.insn   = 0;
    reg_p2.enable = 0;
    reg_p2.active = 0;

    ra = 0;
    rb = 0;
    dbus_data = 0;
    pbus_data = 0;
    printf("pinwheel::reset() done\n");
  }

  //--------------------------------------------------------------------------------

  static logic<32> unpack(logic<32> insn, logic<5> align, logic<32> data) {
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

  static logic<32> alu(logic<32> pc, logic<32> insn, logic<32> ra, logic<32> rb) {
    logic<5> op      = b5(insn, 2);
    logic<3> f3      = b3(insn, 12);
    logic<1> alu_alt = b1(insn, 30);

    if (op == OP_ALU && f3 == 0 && alu_alt) rb = -rb;

    logic<32> imm_i = sign_extend<32>(b12(insn, 20));
    logic<32> imm_u = b20(insn, 12) << 12;

    logic<3>  alu_op = f3;
    logic<32> a = 0, b = 0;
    switch(op) {
      case OP_ALU:     alu_op = f3;  a = ra;  b = rb;      break;
      case OP_ALUI:    alu_op = f3;  a = ra;  b = imm_i;   break;
      case OP_LOAD:    alu_op = f3;  a = ra;  b = rb;      break;
      case OP_STORE:   alu_op = f3;  a = ra;  b = rb;      break;
      case OP_BRANCH:  alu_op = f3;  a = ra;  b = rb;      break;
      case OP_JAL:     alu_op = 0;   a = pc;  b = b32(4);  break;
      case OP_JALR:    alu_op = 0;   a = pc;  b = b32(4);  break;
      case OP_LUI:     alu_op = 0;   a =  0;  b = imm_u;   break;
      case OP_AUIPC:   alu_op = 0;   a = pc;  b = imm_u;   break;
    }

    logic<32> alu_out = 0;
    switch (alu_op) {
      case 0: alu_out = a + b;                 break;
      case 1: alu_out = a << b5(b);            break;
      case 2: alu_out = signed(a) < signed(b); break;
      case 3: alu_out = a < b;                 break;
      case 4: alu_out = a ^ b;                 break;
      case 5: alu_out = alu_alt ? signed(a) >> b5(b) : a >> b5(b); break;
      case 6: alu_out = a | b;                 break;
      case 7: alu_out = a & b;                 break;
    }

    return alu_out;
  }

  //--------------------------------------------------------------------------------

  static cpu_to_mem dbus_out(logic<5> hart, logic<32> insn, logic<32> ra, logic<32> rb) {
    logic<5>  op    = b5(insn, 2);
    logic<3>  f3    = b3(insn, 12);

    logic<32> imm_i = sign_extend<32>(b12(insn, 20));
    logic<32> imm_s = cat(dup<21>(insn[31]), b6(insn, 25), b5(insn, 7));

    logic<32> addr  = ra + ((op == OP_STORE) ? imm_s : imm_i);
    logic<2>  align = b2(addr);
    logic<32> data  = rb << (8 * align);
    logic<4>  mask  = 0b0000;

    if (op == OP_STORE) {
      if (f3 == 0) mask = 0b0001 << align;
      if (f3 == 1) mask = 0b0011 << align;
      if (f3 == 2) mask = 0b1111;
    }

    return { addr, data, mask };
  }

  //--------------------------------------------------------------------------------

  static rbus_read rbus_out_read(logic<5> hart, logic<32> insn) {
    return {
      .raddr1 = cat(hart, b5(insn, 15)),
      .raddr2 = cat(hart, b5(insn, 20)),
    };
  }

  //--------------------------------------------------------------------------------

  static rbus_write rbus_out_write(logic<5> hart, logic<32> insn, logic<5> align, logic<32> data, logic<32> alu_out) {
    logic<5> waddr    = b5(insn, 7);
    logic<5> write_op = b5(insn, 2);

    return {
      .waddr = cat(hart, waddr),
      .wdata = write_op == OP_LOAD ? unpack(insn, align, data) : alu_out,
      .wren  = waddr != 0 && write_op != OP_STORE && write_op != OP_BRANCH,
    };
  }

  //--------------------------------------------------------------------------------

  static cpu_to_mem pbus_out(logic<32> pc) {
    return { pc, 0, 0 };
  }

  //--------------------------------------------------------------------------------

  static logic<32> next_pc(logic<32> pc, logic<32> insn, logic<32> ra, logic<32> rb) {
    logic<5> op = b5(insn, 2);

    if (op == OP_BRANCH) {
      logic<3> f3 = b3(insn, 12);
      logic<1> eq  = ra == rb;
      logic<1> slt = signed(ra) < signed(rb);
      logic<1> ult = ra < rb;

      logic<1> jump_rel = 0;
      switch (f3) {
        case 0: jump_rel =   eq; break;
        case 1: jump_rel =  !eq; break;
        case 2: jump_rel =   eq; break;
        case 3: jump_rel =  !eq; break;
        case 4: jump_rel =  slt; break;
        case 5: jump_rel = !slt; break;
        case 6: jump_rel =  ult; break;
        case 7: jump_rel = !ult; break;
      }

      logic<32> imm_b = cat(dup<20>(insn[31]), insn[7], b6(insn, 25), b4(insn, 8), b1(0));
      return jump_rel ? pc + imm_b : pc + b32(4);
    }
    else if (op == OP_JAL) {
      logic<32> imm_j = cat(dup<12>(insn[31]), b8(insn, 12), insn[20], b10(insn, 21), b1(0));
      return pc + imm_j;
    }
    else if (op == OP_JALR) {
      logic<32> imm_i = sign_extend<32>(b12(insn, 20));
      return ra + imm_i;
    }
    else {
      return pc + 4;
    }
  }

  //--------------------------------------------------------------------------------

  void tick(logic<1> reset_in) {
    if (reset_in) {
      reset();
      return;
    }

    cpu_to_mem to_dbus       = reg_p1.active ? dbus_out(reg_p1.hart, reg_p1.insn, ra, rb) : cpu_to_mem{0};
    rbus_read  to_rbus_read  = reg_p0.active ? rbus_out_read(reg_p0.hart, pbus_data) : rbus_read{0};
    rbus_write to_rbus_write = reg_p2.active ? rbus_out_write(reg_p2.hart, reg_p2.insn, reg_p2.align, dbus_data, reg_p2.alu_out) : rbus_write{0};
    cpu_to_mem to_pbus       = reg_p2.active ? pbus_out(reg_p2.pc) : cpu_to_mem{0};

    auto new_p0_hart   = reg_p2.hart;
    auto new_p0_pc     = reg_p2.pc;
    auto new_p0_enable = reg_p2.enable;
    auto new_p0_active = reg_p2.enable | reg_p2.active;

    reg_p2.hart    wb reg_p1.hart;
    reg_p2.pc      wb reg_p1.active ? next_pc(reg_p1.pc, reg_p1.insn, ra, rb) : b32(0);
    reg_p2.insn    wb reg_p1.insn;
    reg_p2.align   wb b2(to_dbus.addr);
    reg_p2.alu_out wb reg_p1.active ? alu(reg_p1.pc, reg_p1.insn, ra, rb) : b32(0);
    reg_p2.enable  wb reg_p1.enable;
    reg_p2.active  wb reg_p1.active;

    reg_p1.hart    wb reg_p0.hart;
    reg_p1.pc      wb reg_p0.pc;
    reg_p1.insn    wb reg_p0.active ? pbus_data : b32(0);
    reg_p1.enable  wb reg_p0.enable;
    reg_p1.active  wb reg_p0.active;

    reg_p0.hart    wb new_p0_hart;
    reg_p0.pc      wb new_p0_pc;
    reg_p0.enable  wb new_p0_enable;
    reg_p0.active  wb new_p0_active;

    tick_dbus(to_dbus);
    tick_rbus(to_rbus_read, to_rbus_write);
    tick_pbus(to_pbus);
  }

  //--------------------------------------------------------------------------------

  void tick_dbus(cpu_to_mem bus) {
    logic<32> new_data = data_mem[b10(bus.addr, 2)];
    if (bus.mask) {
      if (bus.addr != 0x40000000) {
        if (bus.mask[0]) new_data = (new_data & 0xFFFFFF00) | (bus.data & 0x000000FF);
        if (bus.mask[1]) new_data = (new_data & 0xFFFF00FF) | (bus.data & 0x0000FF00);
        if (bus.mask[2]) new_data = (new_data & 0xFF00FFFF) | (bus.data & 0x00FF0000);
        if (bus.mask[3]) new_data = (new_data & 0x00FFFFFF) | (bus.data & 0xFF000000);
        data_mem[b10(bus.addr, 2)] wb new_data;
      }
    }
    dbus_data wb new_data;
  }

  //--------------------------------------------------------------------------------

  void tick_pbus(cpu_to_mem bus) {
    logic<32> new_data = code_mem[b10(bus.addr, 2)];
    if (bus.mask) {
      new_data = bus.data;
      code_mem[b10(bus.addr, 2)] wb new_data;
    }
    pbus_data wb new_data;
  }

  //--------------------------------------------------------------------------------

  void tick_rbus(rbus_read bus_read, rbus_write bus_write) {
    if (bus_write.wren) {
      regfile[bus_write.waddr] wb bus_write.wdata;
    }
    ra wb regfile[bus_read.raddr1];
    rb wb regfile[bus_read.raddr2];
  }

  //--------------------------------------------------------------------------------

};

  //--------------------------------------------------------------------------------
