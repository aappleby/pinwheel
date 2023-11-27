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
    A_active = 0;
    A_hart = 0;
    A_pc = 0;
    A_insn.raw = 0;

    B_active = 0;
    B_hart = 0;
    B_pc = 0;
    B_insn.raw = 0;

    C_active = 0;
    C_hart = 0;
    //C_pc = 0;
    C_insn.raw = 0;
    C_addr = 0;
    C_result = 0;

    //D_active = 0;
    //D_hart = 0;
    //D_pc = 0;
    //D_insn.raw = 0;

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

  /* metron_noconvert */ logic<32> dbg_decode_imm(rv32_insn insn) const { return decode_imm(insn); }

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
    tla.a_opcode  = (op == RV32I::OP_STORE) ? (bus_size == 2 ? TL::PutFullData : TL::PutPartialData) : TL::Get;
    tla.a_param   = b3(DONTCARE);
    tla.a_size    = bus_size;
    tla.a_source  = b1(DONTCARE);
    tla.a_valid   = (op == RV32I::OP_LOAD) || (op == RV32I::OP_STORE);
    tla.a_ready   = 1;

    return tla;
  }

  //----------------------------------------------------------------------------





















  void tock(logic<1> reset_in, tilelink_d code_tld, tilelink_d data_tld, logic<32> reg1, logic<32> reg2) {

    logic<1> B_swap_pc = B_active && B_insn.r.op == RV32I::OP_SYSTEM && B_insn.r.f3 == RV32I::F3_CSRRW && B_insn.c.csr == 0x801;
    logic<1> C_swap_pc = C_active && C_insn.r.op == RV32I::OP_SYSTEM && C_insn.r.f3 == RV32I::F3_CSRRW && C_insn.c.csr == 0x800;

    //----------------------------------------
    // Vane A receives its instruction from the code bus and issues register
    // reads.

    A_insn.raw = A_active ? code_tld.d_data : b32(0);


    //----------------------------------------
    // Vane B receives its registers from the regfile and updates the data bus.

    B_reg1 = B_insn.r.rs1 ? reg1 : b32(0);
    B_reg2 = B_insn.r.rs2 ? reg2 : b32(0);

    B_imm  = decode_imm(B_insn);
    B_addr = gen_address(B_swap_pc, B_active, B_hart, B_pc, B_insn, B_reg1, B_imm);

    data_tla = gen_bus(B_insn.r.op, B_insn.r.f3, B_addr, B_reg2);

    //----------------------------------------
    // Vane B chooses the instruction address for the _next_ vane A.

    // If vane C contains a CSRRW @ 800 instruction, we override A_pc_next and
    // store the previous value to result.

    // FIXME what if both threads trigger PC swaps at once?
    // - The C one fires first, swapping the other thread.

    logic<1> B_take_branch = take_branch(B_insn.r.f3, B_reg1, B_reg2);


    logic<24> next_pc = B_active ? gen_pc(B_insn.r.op, B_take_branch, B_pc, B_addr, B_imm) : B_pc;


    if (C_swap_pc) {
      A_active_next = b24(C_result) != 0;
      A_hart_next   = b8 (C_result, 24);
      A_pc_next     = b24(C_result);
    }
    else if (B_swap_pc) {
      A_active_next = b24(B_reg1) != 0;
      A_hart_next   = b8 (B_reg1, 24);
      A_pc_next     = b24(B_reg1);
    }
    else {
      A_active_next = B_active;
      A_hart_next   = B_hart;
      A_pc_next     = next_pc;
    }

    logic<32> src_pc = cat(A_hart_next, A_pc_next);

    //----------------------------------------

    logic<1> C_write_code    = !A_active_next && C_active && C_insn.r.op == RV32I::OP_STORE && b4(C_addr, 28) == 0x0;
    logic<1> C_read_regfile  = C_active && C_insn.r.op == RV32I::OP_LOAD && b4(C_addr, 28) == 0xE;
    logic<1> C_write_regfile = C_active && C_insn.r.op == RV32I::OP_STORE && b4(C_addr, 28) == 0xE;
    logic<1> B_read_regfile  = B_active && B_insn.r.op == RV32I::OP_LOAD  && b4(B_addr, 28) == 0xE;

    logic<1> C_read_mem      = C_active && C_insn.r.op == RV32I::OP_LOAD;

    //----------------------------------------
    // Vane B executes its instruction and stores the result in _result.

    B_result = execute(B_active, B_hart, B_pc, B_insn, B_reg1, B_reg2, B_imm);

    //--------------------------------------------------------------------------
    // Regfile write

    logic<32> unpacked_mem = unpack_mem(C_insn.r.f3, C_addr, data_tld.d_data);

    logic<32> writeback = choose_writeback(
      C_swap_pc,
      C_read_regfile,
      C_read_mem,
      src_pc,
      C_active, C_insn, C_addr, C_result,
      reg2,
      unpacked_mem);

    //--------------------------------------------------------------------------
    // Regfile write

    reg_if = reg_bus(
      B_read_regfile,
      C_write_regfile,
      A_active, A_insn, A_hart,
      B_active, B_insn, B_hart, B_addr,
      C_active, C_insn, C_hart, C_addr,
      writeback);


    //--------------------------------------------------------------------------
    // Code bus read/write

    code_tla = code_bus(
      C_write_code,
      A_active_next, A_pc_next,
      C_active, C_insn, C_addr, C_result);

    //----------

    tick(reset_in);
  }

  //----------------------------------------------------------------------------


























private:

  //--------------------------------------------------------------------------

  static logic<32> gen_address(logic<1> swap_b, logic<1> B_active, logic<8> B_hart, logic<24> B_pc, rv32_insn B_insn, logic<32> B_reg1, logic<32> B_imm) {
    logic<32> B_addr;

    if (swap_b) {
      B_addr = cat(B_hart, B_pc);
    }
    else if (B_active) {
      B_addr = b32(B_reg1 + B_imm);
    }
    else {
      B_addr = b32(DONTCARE);
    }

    return B_addr;
  }

  //--------------------------------------------------------------------------

  static logic<32> execute(logic<1> B_active, logic<8> B_hart, logic<24> B_pc, rv32_insn B_insn, logic<32> B_reg1, logic<32> B_reg2, logic<32> B_imm) {
    logic<32> B_result = b32(DONTCARE);
    if (B_active) {
      switch(B_insn.r.op) {
        case RV32I::OP_OPIMM:  B_result = execute_alu(B_insn, B_reg1, B_imm);  break;
        case RV32I::OP_OP:     B_result = execute_alu(B_insn, B_reg1, B_reg2); break;
        case RV32I::OP_SYSTEM: B_result = execute_system(B_hart, B_pc, B_insn, B_reg1); break;
        case RV32I::OP_BRANCH: B_result = b32(DONTCARE);     break;
        case RV32I::OP_JAL:    B_result = b24(B_pc) + 4;     break;
        case RV32I::OP_JALR:   B_result = b24(B_pc) + 4;     break;
        case RV32I::OP_LUI:    B_result = B_imm;             break;
        case RV32I::OP_AUIPC:  B_result = b24(B_pc) + B_imm; break;
        case RV32I::OP_LOAD:   B_result = b32(DONTCARE);     break;
        case RV32I::OP_STORE:  B_result = B_reg2;            break;
        default:                B_result = b32(DONTCARE);     break;
      }
    }
    return B_result;
  }

  //--------------------------------------------------------------------------

  static logic<32> choose_writeback(
    logic<1> swap_c,
    logic<1> C_read_regfile,
    logic<1> C_read_mem,
    logic<32> src_pc,
    logic<1> C_active, rv32_insn C_insn, logic<32> C_addr, logic<32> C_result,
    logic<32> raw_reg2,
    logic<32> unpacked_mem)
  {
    if (swap_c) {
      // If we're switching secondary threads, we write the previous secondary
      // thread back to the primary thread's regfile.
      return src_pc;
    }
    else if (C_read_regfile) {
      return raw_reg2;
    }
    else if (C_read_mem) {
      // A memory read replaces _result with the unpacked value on the data bus.
      return unpacked_mem;
    }
    else if (C_active) {
      return C_result;
    }
    else {
      return b32(DONTCARE);
    }
  }


  //--------------------------------------------------------------------------

  static regfile_if reg_bus(
    logic<1> B_read_regfile,
    logic<1> C_write_regfile,
    logic<1> A_active, rv32_insn A_insn, logic<8> A_hart,
    logic<1> B_active, rv32_insn B_insn, logic<8> B_hart, logic<32> B_addr,
    logic<1> C_active, rv32_insn C_insn, logic<8> C_hart, logic<32> C_addr,
    logic<32> writeback)
  {
    regfile_if reg_if = {};

    if (C_write_regfile) {
      // A memory-mapped regfile write overrides the normal write.
      reg_if.waddr = b8(C_addr >> 2);
      reg_if.wdata = writeback;
      reg_if.wren  = 1;
    } else if (C_active) {
      // Vane C writes _result to _rd if the thread is active and _rd != 0.
      reg_if.waddr = cat(C_hart, b5(C_insn.r.rd));
      reg_if.wdata = writeback;
      reg_if.wren  = C_insn.r.rd && C_insn.r.op != RV32I::OP_STORE && C_insn.r.op != RV32I::OP_BRANCH;
    }
    else {
      reg_if.waddr = b13(DONTCARE);
      reg_if.wdata = b32(DONTCARE);
      reg_if.wren  = 0;
    }


    // If vane A is idle, vane B uses vane A's regfile slot to do an additional
    // regfile read. This is used for memory-mapped regfile reading.

    if (A_active) {
      reg_if.raddr1 = cat(A_hart, b5(A_insn.r.rs1));
      reg_if.raddr2 = cat(A_hart, b5(A_insn.r.rs2));
    }
    else if (B_read_regfile) {
      reg_if.raddr1 = b13(DONTCARE);
      reg_if.raddr2 = b13(B_addr >> 2);
    }
    else {
      reg_if.raddr1 = b13(DONTCARE);
      reg_if.raddr2 = b13(DONTCARE);
    }

    return reg_if;
  }

  //----------------------------------------------------------------------------
  // If the other thread is idle, vane C can override the code bus read to
  // write to code memory.
  // We can _not_ read code memory here as the read would come back too late
  // to write it to the regfile.

  static tilelink_a code_bus(logic<1> C_write_code, logic<1> A_active_next, logic<24> A_pc_next, logic<1> C_active, rv32_insn C_insn, logic<32> C_addr, logic<32> C_result) {
    tilelink_a tla;
    if (C_write_code) {
      tla = gen_bus(C_insn.r.op, C_insn.r.f3, C_addr, C_result);
    }
    else {
      tla = gen_bus(RV32I::OP_LOAD, 2, b32(A_pc_next), 0);
    }
    return tla;
  }

  //----------------------------------------------------------------------------

  static logic<32> unpack_mem(logic<3> f3, logic<32> addr, logic<32> mem) {
    if (addr[0]) mem = mem >> 8;
    if (addr[1]) mem = mem >> 16;
    switch (f3) {
      case RV32I::F3_LB:  mem = sign_extend<32>( b8(mem)); break;
      case RV32I::F3_LH:  mem = sign_extend<32>(b16(mem)); break;
      case RV32I::F3_LW:  mem = mem; break;
      case RV32I::F3_LD:  mem = mem; break;
      case RV32I::F3_LBU: mem = zero_extend<32>( b8(mem)); break;
      case RV32I::F3_LHU: mem = zero_extend<32>(b16(mem)); break;
      case RV32I::F3_LWU: mem = mem; break;
      case RV32I::F3_LDU: mem = mem; break;
    }
    return mem;
  }

  //----------------------------------------------------------------------------

  static logic<32> execute_system(logic<8> B_hart, logic<24> B_pc, rv32_insn B_insn, logic<32> B_reg1) {
    // FIXME need a good error if case is missing an expression
    logic<32> B_result = b32(DONTCARE);
    switch(B_insn.r.f3) {
      case RV32I::F3_CSRRW: {
        if (B_insn.c.csr == 0x800)          B_result = B_reg1;
        if (B_insn.c.csr == 0x801)          B_result = cat(B_hart, B_pc);
        if (B_insn.c.csr == RV32I::MHARTID) B_result = B_hart;
        break;
      }
      case RV32I::F3_CSRRS: {
        if (B_insn.c.csr == RV32I::MHARTID) B_result = B_hart;
        break;
      }
      case RV32I::F3_CSRRC:  B_result = b32(DONTCARE); break;
      case RV32I::F3_CSRRWI: B_result = b32(DONTCARE); break;
      case RV32I::F3_CSRRSI: B_result = b32(DONTCARE); break;
      case RV32I::F3_CSRRCI: B_result = b32(DONTCARE); break;
    }
    return B_result;
  }

  //----------------------------------------------------------------------------

  static logic<24> gen_pc(logic<7> op, logic<1> take_branch, logic<24> pc, logic<32> addr, logic<32> imm) {
    logic<24> pc_jump = pc + imm;
    logic<24> pc_next = pc + 4;
    logic<24> pc_addr = b24(addr);

    switch(op) {
      case RV32I::OP_BRANCH: return take_branch ? pc_jump : pc_next;
      case RV32I::OP_JAL:    return pc_jump;
      case RV32I::OP_JALR:   return pc_addr;
      case RV32I::OP_LUI:    return pc_next;
      case RV32I::OP_AUIPC:  return pc_next;
      case RV32I::OP_LOAD:   return pc_next;
      case RV32I::OP_STORE:  return pc_next;
      case RV32I::OP_SYSTEM: return pc_next;
      case RV32I::OP_OPIMM:  return pc_next;
      case RV32I::OP_OP:     return pc_next;
      default:               return b24(DONTCARE);
    }
  }

  //----------------------------------------------------------------------------

  static logic<1> take_branch(logic<3> f3, logic<32> reg1, logic<32> reg2) {
    logic<1> eq  = reg1 == reg2;
    logic<1> slt = signed(reg1) < signed(reg2);
    logic<1> ult = reg1 < reg2;

    switch (f3) {
      case RV32I::F3_BEQ:  return   eq;
      case RV32I::F3_BNE:  return  !eq;
      case RV32I::F3_BEQU: return   eq;
      case RV32I::F3_BNEU: return  !eq;
      case RV32I::F3_BLT:  return  slt;
      case RV32I::F3_BGE:  return !slt;
      case RV32I::F3_BLTU: return  ult;
      case RV32I::F3_BGEU: return !ult;
      default:             return    0;
    }
  }

  //----------------------------------------------------------------------------

  static logic<32> decode_imm(rv32_insn insn2) {
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

    switch(insn2.r.op) {
      case RV32I::OP_LOAD:   return imm_i;
      case RV32I::OP_OPIMM:  return imm_i;
      case RV32I::OP_AUIPC:  return imm_u;
      case RV32I::OP_STORE:  return imm_s;
      case RV32I::OP_OP:     return imm_i;
      case RV32I::OP_LUI:    return imm_u;
      case RV32I::OP_BRANCH: return imm_b;
      case RV32I::OP_JALR:   return imm_i;
      case RV32I::OP_JAL:    return imm_j;
      default:               return b32(DONTCARE);
    }
  }

  //----------------------------------------------------------------------------

  static logic<32> execute_alu(rv32_insn insn, logic<32> alu1, logic<32> alu2) {

    logic<32> result;
    switch (insn.r.f3) {
      case RV32I::F3_ADDSUB: result = (insn.r.op == RV32I::OP_OP && insn.r.f7 == 32) ? alu1 - alu2 : alu1 + alu2; break;
      case RV32I::F3_SL:     result = alu1 << b5(alu2); break;
      case RV32I::F3_SLT:    result = signed(alu1) < signed(alu2); break;
      case RV32I::F3_SLTU:   result = alu1 < alu2; break;
      case RV32I::F3_XOR:    result = alu1 ^ alu2; break;
      case RV32I::F3_SR:     result = insn.r.f7 == 32 ? signed(alu1) >> b5(alu2) : alu1 >> b5(alu2); break;
      case RV32I::F3_OR:     result = alu1 | alu2; break;
      case RV32I::F3_AND:    result = alu1 & alu2; break;
      default:               result = 0; break;
    }
    return result;
  }

  //----------------------------------------------------------------------------

  void tick(logic<1> reset_in)
  {
    if (reset_in) {
      A_active = 1; A_hart = 0; A_pc = 0x00000004;
      B_active = 0; B_hart = 0; B_pc = 0x00000000;
      C_active = 0; C_hart = 0; //C_pc = 0x00000000;
      //D_active = 0; D_hart = 0; D_pc = 0x00000000;
      ticks = 0x00000000;
    }
    else {
      //D_active = C_active;
      //D_hart   = C_hart;
      //D_pc     = C_pc;
      //D_insn   = C_insn;
      //D_result = C_result;

      C_active = B_active;
      C_hart   = B_hart;
      //C_pc     = B_pc;
      C_insn   = B_insn;
      C_addr   = B_addr;
      C_result = B_result;

      B_active = A_active;
      B_hart   = A_hart;
      B_pc     = A_pc;
      B_insn   = A_insn;

      A_active = A_active_next;
      A_hart   = A_hart_next;
      A_pc     = A_pc_next;

      ticks    = ticks + 1;
    }
  }

public:

  //----------------------------------------
  // Internal signals and registers

  /* metron_internal */ logic<1>  A_active_next;
  /* metron_internal */ logic<8>  A_hart_next;
  /* metron_internal */ logic<24> A_pc_next;

  /* metron_internal */ logic<1>  A_active;
  /* metron_internal */ logic<8>  A_hart;
  /* metron_internal */ logic<24> A_pc;
  /* metron_internal */ rv32_insn A_insn;

  /* metron_internal */ logic<1>  B_active;
  /* metron_internal */ logic<8>  B_hart;
  /* metron_internal */ logic<24> B_pc;
  /* metron_internal */ rv32_insn B_insn;
  /* metron_internal */ logic<32> B_reg1; // not essential
  /* metron_internal */ logic<32> B_reg2; // not essential
  /* metron_internal */ logic<32> B_imm;  // not essential
  /* metron_internal */ logic<32> B_addr;
  /* metron_internal */ logic<32> B_result;

  /* metron_internal */ logic<1>  C_active;
  /* metron_internal */ logic<8>  C_hart;
  ///* metron_internal */ logic<24> C_pc;
  /* metron_internal */ rv32_insn C_insn;
  /* metron_internal */ logic<32> C_addr;
  /* metron_internal */ logic<32> C_result;

  // These registers aren't actually needed, but they make debugging easier.
  ///* metron_internal */ logic<1>  D_active;
  ///* metron_internal */ logic<8>  D_hart;
  ///* metron_internal */ logic<24> D_pc;
  ///* metron_internal */ rv32_insn D_insn;
  ///* metron_internal */ logic<32> D_result;

  /* metron_internal */ logic<32> ticks;

};

/* verilator lint_on UNUSEDSIGNAL */
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_PINWHEEL_CORE_H
