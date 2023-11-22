#ifndef PINWHEEL_RTL_PINWHEEL_CORE_H
#define PINWHEEL_RTL_PINWHEEL_CORE_H

// FIXME need to do struct/union thing for rv32 instructions so we don't have
// so much duplicate decoding crap

#include "metron/metron_tools.h"
#include "pinwheel/tools/regfile_if.h"
#include "pinwheel/tools/tilelink.h"
#include "pinwheel/tools/riscv_constants.h"

#include <assert.h>

// CSR 0x800 - swap secondary thread
// CSR 0x801 - swap current thread

// 0xE0000000 - mapped regfile base address

// FIXME where does the old PC go when we swap the current thread?

//------------------------------------------------------------------------------
/* verilator lint_off UNUSEDSIGNAL */

class pinwheel_core {
public:

  pinwheel_core() {
    A_hart = 0;
    A_pc = 0;
    A_insn.raw = 0;
    B_hart = 0;
    B_pc = 0;
    B_insn.raw = 0;
    C_hart = 0;
    C_pc = 0;
    C_insn.raw = 0;
    C_addr = 0;
    C_result = 0;
    D_hart = 0;
    D_pc = 0;
    D_insn.raw = 0;
    ticks = 0;

    code_tla.a_opcode  = TL::Invalid;
    code_tla.a_param   = b3(DONTCARE);
    code_tla.a_size    = b3(DONTCARE);
    code_tla.a_source  = b1(DONTCARE);
    code_tla.a_address = b32(DONTCARE);
    code_tla.a_mask    = b4(DONTCARE);
    code_tla.a_data    = b32(DONTCARE);
    code_tla.a_valid   = b1(DONTCARE);
    code_tla.a_ready   = b1(DONTCARE);

    data_tla.a_opcode  = TL::Invalid;
    data_tla.a_param   = b3(DONTCARE);
    data_tla.a_size    = b3(DONTCARE);
    data_tla.a_source  = b1(DONTCARE);
    data_tla.a_address = b32(DONTCARE);
    data_tla.a_mask    = b4(DONTCARE);
    data_tla.a_data    = b32(DONTCARE);
    data_tla.a_valid   = b1(DONTCARE);
    data_tla.a_ready   = b1(DONTCARE);
  }

  /* metron_noconvert */ logic<32> dbg_decode_imm(rv32_insn insn) const { return decode_imm2(insn); }

  //----------------------------------------

  // FIXME yosys does not like this as a local variable
  tilelink_a data_tla;
  tilelink_a code_tla;
  regfile_if reg_if;

  static tilelink_a gen_bus(logic<7> op, logic<3> f3, logic<32> addr, logic<32> reg2) {
    tilelink_a tla;

    logic<3> bus_size = b3(DONTCARE);
    logic<4> mask_b   = 0;

    if (f3 == 0) { mask_b = 0b0001; bus_size = 0; }
    if (f3 == 1) { mask_b = 0b0011; bus_size = 1; }
    if (f3 == 2) { mask_b = 0b1111; bus_size = 2; }
    if (addr[0]) mask_b = mask_b << 1;
    if (addr[1]) mask_b = mask_b << 2;

    tla.a_address = addr;
    tla.a_data    = (reg2 << ((addr & 3) * 8));
    tla.a_mask    = mask_b;
    tla.a_opcode  = (op == RV32I::OP2_STORE) ? (bus_size == 2 ? TL::PutFullData : TL::PutPartialData) : TL::Get;
    tla.a_param   = b3(DONTCARE);
    tla.a_size    = bus_size;
    tla.a_source  = b1(DONTCARE);
    tla.a_valid   = (op == RV32I::OP2_LOAD) || (op == RV32I::OP2_STORE);
    tla.a_ready   = 1;

    return tla;
  }

  //----------------------------------------------------------------------------

  void tock(logic<1> reset_in, tilelink_d code_tld, tilelink_d data_tld, logic<32> reg1, logic<32> reg2) {

    //----------
    // Vane A receives its instruction from the code bus and issues register
    // reads.

    A_insn.raw = b24(A_pc) ? code_tld.d_data : b32(0);

    reg_if.raddr1 = b8(((A_pc >> 24) << 5) | A_insn.r.rs1);
    reg_if.raddr2 = b8(((A_pc >> 24) << 5) | A_insn.r.rs2);

    //----------
    // Vane B receives its registers from the regfile and updates the data bus.

    B_reg1 = B_insn.r.rs1 ? reg1 : b32(0);
    B_reg2 = B_insn.r.rs2 ? reg2 : b32(0);

    B_imm  = decode_imm2(B_insn);
    B_addr = b32(B_reg1 + B_imm);
    data_tla = gen_bus(B_insn.r.op, B_insn.r.f3, B_addr, B_reg2);

    // If vane A is idle, vane B uses vane A's regfile slot to do an additional
    // regfile read.

    if (b24(A_pc) == 0) {
      reg_if.raddr1 = b10(DONTCARE);
      reg_if.raddr2 = b10(B_addr >> 2);
    }

    //----------
    // Vane B executes an instruction.

    switch(B_insn.r.op) {
      case RV32I::OP2_OPIMM:  B_result = execute_alu   (B_insn, B_reg1, B_imm);  break;
      case RV32I::OP2_OP:     B_result = execute_alu   (B_insn, B_reg1, B_reg2); break;
      case RV32I::OP2_SYSTEM: B_result = execute_system(B_insn); break;
      case RV32I::OP2_BRANCH: B_result = b32(DONTCARE); break;
      case RV32I::OP2_JAL:    B_result = B_pc + 4;      break;
      case RV32I::OP2_JALR:   B_result = B_pc + 4;      break;
      case RV32I::OP2_LUI:    B_result = B_imm;         break;
      case RV32I::OP2_AUIPC:  B_result = B_pc + B_imm;  break;
      case RV32I::OP2_LOAD:   B_result = B_addr;        break;
      case RV32I::OP2_STORE:  B_result = B_reg2;        break;
      default:                B_result = b32(DONTCARE); break;
    }

    //----------
    // Next instruction selection

    logic<24> pc = b24(B_pc);
    if (pc) {
      logic<1> eq  = B_reg1 == B_reg2;
      logic<1> slt = signed(B_reg1) < signed(B_reg2);
      logic<1> ult = B_reg1 < B_reg2;
      logic<1> take_branch = 0;

      switch (B_insn.r.f3) {
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

      switch(B_insn.r.op) {
        case RV32I::OP2_BRANCH: pc = take_branch ? pc + B_imm : pc + 4; break;
        case RV32I::OP2_JAL:    pc = pc + B_imm; break;
        case RV32I::OP2_JALR:   pc = B_addr; break;
        case RV32I::OP2_LUI:    pc = pc + 4; break;
        case RV32I::OP2_AUIPC:  pc = pc + 4; break;
        case RV32I::OP2_LOAD:   pc = pc + 4; break;
        case RV32I::OP2_STORE:  pc = pc + 4; break;
        case RV32I::OP2_SYSTEM: pc = pc + 4; break;
        case RV32I::OP2_OPIMM:  pc = pc + 4; break;
        case RV32I::OP2_OP:     pc = pc + 4; break;
      }
    }

    logic<8>  A_hart_next = b8(B_pc, 24);
    logic<32> A_pc_next   = cat(A_hart_next, pc);

    // If we write to CSR 0x801, we swap the current thread's PC with the
    // register value. This has the effect of 'yielding' to the new thread.
    if (B_insn.r.op == RV32I::OP2_SYSTEM && B_insn.r.f3 == RV32I::F3_CSRRW && B_insn.c.csr == 0x801) {
      logic<32> temp = B_result;
      B_result    = A_pc_next;
      A_hart_next = b8(temp, 24);
      A_pc_next   = temp;
    }

    //----------
    // PC hackery to swap threads

    // FIXME what if both threads trigger these two PC swaps at once?

    // If we write to CSR 0x800, we swap the secondary thread's PC with the
    // register value.
    if (C_insn.r.op == RV32I::OP2_SYSTEM && C_insn.r.f3 == RV32I::F3_CSRRW && C_insn.c.csr == 0x800) {
      logic<32> temp = C_result;
      C_result   = A_pc_next;
      A_hart_next = b8(C_result, 24);
      A_pc_next   = temp;
    }

    //--------------------------------------------------------------------------
    // Vane C unpacks the result of the memory read.

    if (C_insn.r.op == RV32I::OP2_LOAD) {
      logic<32> C_mem = data_tld.d_data;
      if (C_result[0]) C_mem = C_mem >> 8;
      if (C_result[1]) C_mem = C_mem >> 16;
      switch (C_insn.r.f3) {
        case 0:  C_mem = sign_extend<32>( b8(C_mem)); break;
        case 1:  C_mem = sign_extend<32>(b16(C_mem)); break;
        case 4:  C_mem = zero_extend<32>( b8(C_mem)); break;
        case 5:  C_mem = zero_extend<32>(b16(C_mem)); break;
      }

      C_result = C_mem;
    }

    //--------------------------------------------------------------------------
    // Regfile write

    // Normally we write 'result' to 'rd' if rd != 0 and the thread is active.

    logic<1> C_regfile_cs = (b4(C_addr, 28) == 0xE) && (C_insn.r.op == RV32I::OP2_LOAD || C_insn.r.op == RV32I::OP2_STORE);

    logic<8> C_waddr = cat(b3(C_pc, 24), b5(C_insn.r.rd));

    if (C_regfile_cs && C_insn.r.op == RV32I::OP2_LOAD) {
      C_result = reg2;
    }
    if (C_regfile_cs && C_insn.r.op == RV32I::OP2_STORE) {
      C_waddr = b10(C_addr >> 2);
    }

    reg_if.waddr = C_waddr;
    reg_if.wdata = C_result;
    reg_if.wren  = 1;

    if (C_insn.r.op == RV32I::OP2_STORE)  reg_if.wren  = C_regfile_cs;
    if (C_insn.r.op == RV32I::OP2_BRANCH) reg_if.wren  = 0;

    //--------------------------------------------------------------------------
    // Code bus read/write

    // We can write code memory in phase C if the other thread is idle.
    // We can _not_ read code memory here as the read would come back too late
    // to write it to the regfile.

    logic<1> C_code_cs =
      (C_insn.r.op == RV32I::OP2_STORE)
      && C_pc
      && b4(C_addr, 28) == 0x0
      && b24(A_pc_next) == 0;

    if (C_code_cs) {
      code_tla = gen_bus(C_insn.r.op, C_insn.r.f3, C_addr, C_result);
    }
    else {
      code_tla = gen_bus(RV32I::OP2_LOAD, 2, A_pc_next, 0);
    }

    //----------

    tick(reset_in, A_hart_next, A_pc_next, B_result, B_addr);
  }

  //----------------------------------------------------------------------------

private:

  void tick(logic<1>  reset_in,
            logic<8>  A_hart_next,
            logic<32> A_pc_next,
            logic<32> B_result,
            logic<32> B_addr)
  {
    if (reset_in) {
      A_hart = 0; A_pc = 0x00000004;
      B_hart = 0; B_pc = 0x00000000;
      C_hart = 0; C_pc = 0x00000000;
      D_hart = 0; D_pc = 0x00000000;
      ticks    = 0x00000000;
    }
    else {
      D_hart   = C_hart;
      D_pc     = C_pc;
      D_insn   = C_insn;
      D_result = C_result;

      C_hart   = B_hart;
      C_pc     = B_pc;
      C_insn   = B_insn;
      C_addr   = B_addr;
      C_result = B_result;

      B_hart   = A_hart;
      B_pc     = A_pc;
      B_insn   = A_insn;

      A_hart   = A_hart_next;
      A_pc     = A_pc_next;

      ticks    = ticks + 1;
    }
  }

  //----------------------------------------

  logic<32> decode_imm2(rv32_insn insn2) const {
    logic<32> imm_i = sign_extend<32>(b12(insn2.i.imm_11_0));
    logic<32> imm_s = sign_extend<32>(cat(b7(insn2.s.imm_11_5), b5(insn2.s.imm_4_0)));
    logic<32> imm_u = cat(b20(insn2.u.imm_31_12), b12(0));

    logic<32> imm_b = sign_extend<32>(cat(
      b1(insn2.b.imm_12),
      b1(insn2.b.imm_11),
      b6(insn2.b.imm_10_5),
      b4(insn2.b.imm_4_1),
      b1(0)
    ));

    logic<32> imm_j = sign_extend<32>(cat(
      b1 (insn2.j.imm_20),
      b8 (insn2.j.imm_19_12),
      b1 (insn2.j.imm_11),
      b10(insn2.j.imm_10_1),
      b1 (0)
    ));

    logic<32> result;
    switch(insn2.r.op) {
      case RV32I::OP2_LOAD:   result = imm_i; break;
      case RV32I::OP2_OPIMM:  result = imm_i; break;
      case RV32I::OP2_AUIPC:  result = imm_u; break;
      case RV32I::OP2_STORE:  result = imm_s; break;
      case RV32I::OP2_OP:     result = imm_i; break;
      case RV32I::OP2_LUI:    result = imm_u; break;
      case RV32I::OP2_BRANCH: result = imm_b; break;
      case RV32I::OP2_JALR:   result = imm_i; break;
      case RV32I::OP2_JAL:    result = imm_j; break;
      default:                result = 0;     break;
    }
    return result;
  }

  //----------------------------------------

  logic<32> execute_alu(rv32_insn insn, logic<32> alu1, logic<32> alu2) const {

    if (insn.r.op == RV32I::OP2_OP && insn.r.f3 == 0 && insn.r.f7 == 32) alu2 = -alu2;

    logic<32> result;
    switch (insn.r.f3) {
      case 0:  result = alu1 + alu2; break;
      case 1:  result = alu1 << b5(alu2); break;
      case 2:  result = signed(alu1) < signed(alu2); break;
      case 3:  result = alu1 < alu2; break;
      case 4:  result = alu1 ^ alu2; break;
      case 5:  result = insn.r.f7 == 32 ? signed(alu1) >> b5(alu2) : alu1 >> b5(alu2); break;
      case 6:  result = alu1 | alu2; break;
      case 7:  result = alu1 & alu2; break;
      default: result = 0; break;
    }
    return result;
  }

  //----------------------------------------

  logic<32> execute_system(rv32_insn insn) const {

    // FIXME need a good error if case is missing an expression
    logic<32> result = 0;
    switch(insn.r.f3) {
      case RV32I::F3_CSRRW: {
        result = B_reg1;
        break;
      }
      case RV32I::F3_CSRRS: {
        if (insn.c.csr == 0xF14) result = b8(B_pc, 24);
        break;
      }
      case RV32I::F3_CSRRC:  result = 0; break;
      case RV32I::F3_CSRRWI: result = 0; break;
      case RV32I::F3_CSRRSI: result = 0; break;
      case RV32I::F3_CSRRCI: result = 0; break;
    }
    return result;
  }

public:

  //----------------------------------------
  // Internal signals and registers

  /* metron_internal */ logic<8>  A_hart;
  /* metron_internal */ logic<32> A_pc;
  /* metron_internal */ rv32_insn A_insn;

  /* metron_internal */ logic<8>  B_hart;
  /* metron_internal */ logic<32> B_pc;
  /* metron_internal */ rv32_insn B_insn;
  /* metron_internal */ logic<32> B_reg1;
  /* metron_internal */ logic<32> B_reg2;
  /* metron_internal */ logic<32> B_imm;
  /* metron_internal */ logic<32> B_addr;
  /* metron_internal */ logic<32> B_result;

  /* metron_internal */ logic<8>  C_hart;
  /* metron_internal */ logic<32> C_pc;
  /* metron_internal */ rv32_insn C_insn;
  /* metron_internal */ logic<32> C_addr;
  /* metron_internal */ logic<32> C_result;

  // These registers aren't actually needed, but they make debugging easier.
  /* metron_internal */ logic<8>  D_hart;
  /* metron_internal */ logic<24> D_pc;
  /* metron_internal */ rv32_insn D_insn;
  /* metron_internal */ logic<32> D_result;

  /* metron_internal */ logic<32> ticks;

};

/* verilator lint_on UNUSEDSIGNAL */
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_PINWHEEL_CORE_H
