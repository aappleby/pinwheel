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
    A_hpc = 0;
    A_insn.raw = 0;

    B_hpc = 0;
    B_insn.raw = 0;

    C_hpc = 0;
    C_insn.raw = 0;
    C_addr = 0;
    C_result = 0;

    D_hpc = 0;
    //D_insn.raw = 0;
    //D_writeback = 0;
    D_waddr = 0;
    D_wdata = 0;
    D_wren = 0;

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

  //----------------------------------------------------------------------------

  void tock(logic<1> reset_in, tilelink_d code_tld, tilelink_d data_tld, logic<32> reg1, logic<32> reg2) {

    logic<8> A_hart = b8(A_hpc, 24);
    logic<8> B_hart = b8(B_hpc, 24);
    logic<8> C_hart = b8(C_hpc, 24);
    logic<8> D_hart = b8(D_hpc, 24);

    logic<24> A_pc = b24(A_hpc);
    logic<24> B_pc = b24(B_hpc);
    logic<24> C_pc = b24(C_hpc);
    logic<24> D_pc = b24(D_hpc);

    logic<1> A_active = A_pc != 0;
    logic<1> B_active = B_pc != 0;
    logic<1> C_active = C_pc != 0;
    logic<1> D_active = D_pc != 0;

    logic<1> B_yield = B_active && B_insn.r.op == RV32I::OP_SYSTEM && B_insn.r.f3 == RV32I::F3_CSRRW && B_insn.c.csr == 0x801;
    logic<1> C_yield = C_active && C_insn.r.op == RV32I::OP_SYSTEM && C_insn.r.f3 == RV32I::F3_CSRRW && C_insn.c.csr == 0x801;

    logic<1> B_swap = B_active && B_insn.r.op == RV32I::OP_SYSTEM && B_insn.r.f3 == RV32I::F3_CSRRW && B_insn.c.csr == 0x800;
    logic<1> C_swap = C_active && C_insn.r.op == RV32I::OP_SYSTEM && C_insn.r.f3 == RV32I::F3_CSRRW && C_insn.c.csr == 0x800;

    logic<1> C_read_regfile  = C_active && C_insn.r.op == RV32I::OP_LOAD  && b4(C_addr, 28) == 0xE;
    logic<1> C_read_mem      = C_active && C_insn.r.op == RV32I::OP_LOAD;

    logic<1> C_write_code    = C_active && C_insn.r.op == RV32I::OP_STORE && b4(C_addr, 28) == 0x0;
    logic<1> C_write_regfile = C_active && C_insn.r.op == RV32I::OP_STORE && b4(C_addr, 28) == 0xE;

    //----------------------------------------
    // Vane A receives its instruction from the code bus and issues register
    // reads.

    A_insn.raw = A_active ? code_tld.d_data : b32(0);


    //----------------------------------------

    logic<32> B_reg1 = B_insn.r.rs1 ? reg1 : b32(0);
    logic<32> B_reg2 = B_insn.r.rs2 ? reg2 : b32(0);
    logic<32> B_imm  = decode_imm(B_insn);
    logic<32> B_addr = b32(B_reg1 + B_imm);

    logic<1> B_read_regfile  = B_active && B_insn.r.op == RV32I::OP_LOAD  && b4(B_addr, 28) == 0xE;

    //----------------------------------------

    logic<1>  B_take_branch = take_branch(B_insn.r.f3, B_reg1, B_reg2);
    logic<32> B_pc_jump = B_pc + B_imm;
    logic<32> B_pc_next = B_pc + 4;
    logic<24> B_next_pc;

    switch(B_insn.r.op) {
      case RV32I::OP_BRANCH: B_next_pc = B_take_branch ? B_pc_jump : B_pc_next; break;
      case RV32I::OP_JAL:    B_next_pc = B_pc_jump; break;
      case RV32I::OP_JALR:   B_next_pc = B_addr; break;
      case RV32I::OP_LUI:    B_next_pc = B_pc_next; break;
      case RV32I::OP_AUIPC:  B_next_pc = B_pc_next; break;
      case RV32I::OP_LOAD:   B_next_pc = B_pc_next; break;
      case RV32I::OP_STORE:  B_next_pc = B_pc_next; break;
      case RV32I::OP_SYSTEM: B_next_pc = B_pc_next; break;
      case RV32I::OP_OPIMM:  B_next_pc = B_pc_next; break;
      case RV32I::OP_OP:     B_next_pc = B_pc_next; break;
      default:               B_next_pc = b24(DONTCARE); break;
    }

    logic<32> B_hpc_next = cat(B_hart, B_next_pc);

    //----------------------------------------
    // Vane B executes its instruction and stores the result in _result.

    logic<32> B_result;
    switch(B_insn.r.op) {
      case RV32I::OP_OPIMM:  B_result = execute_alu(B_insn, B_reg1, B_imm); break;
      case RV32I::OP_OP:     B_result = execute_alu(B_insn, B_reg1, B_reg2); break;
      case RV32I::OP_SYSTEM: B_result = execute_system(B_hart, B_insn, B_reg1); break;
      case RV32I::OP_BRANCH: B_result = b32(DONTCARE); break;
      case RV32I::OP_JAL:    B_result = B_pc_next; break;
      case RV32I::OP_JALR:   B_result = B_pc_next; break;
      case RV32I::OP_LUI:    B_result = B_imm; break;
      case RV32I::OP_AUIPC:  B_result = B_pc + B_imm; break;
      case RV32I::OP_LOAD:   B_result = b32(DONTCARE); break;
      case RV32I::OP_STORE:  B_result = B_reg2; break;
      default:               B_result = b32(DONTCARE); break;
    }

    // If this thread is yielding, we store where it _would've_ gone in B_result
    // so we can write it to the regfile in phase C.
    if (B_yield) B_result = B_hpc_next;

    // If we're going to swap secondary threads in phase C, we need to stash
    // reg1 in B_result so we can use it above.
    if (B_swap)  B_result = B_reg1;

    //----------------------------------------
    // If both threads trigger PC swaps at once, the C one fires first.

    logic<32> A_hpc_next;

    if (C_swap) {
      A_hpc_next = C_result;
    }
    else if (B_yield) {
      A_hpc_next = B_reg1;
    }
    else {
      A_hpc_next = B_hpc_next;
    }

    logic<1> A_active_next = b24(A_hpc_next) != 0;

    //--------------------------------------------------------------------------
    // Regfile write

    logic<32> C_mem = unpack_mem(C_insn.r.f3, C_addr, data_tld.d_data);

    logic<32> C_writeback;

    if (C_yield) {
      C_writeback = C_result;
    }
    else if (C_swap) {
      // If we're switching secondary threads, we write the previous secondary
      // thread back to the primary thread's regfile.
      C_writeback = B_hpc_next;
    }
    else if (C_read_regfile) {
      C_writeback = reg2;
    }
    else if (C_read_mem) {
      // A memory read replaces _result with the unpacked value on the data bus.
      C_writeback = C_mem;
    }
    else if (C_active) {
      C_writeback = C_result;
    }
    else {
      C_writeback = b32(DONTCARE);
    }

    //--------------------------------------------------------------------------
    // Regfile write

    logic<13> C_waddr;
    logic<32> C_wdata;
    logic<1>  C_wren;

    if (C_write_regfile) {
      // A memory-mapped regfile write overrides the normal write.
      C_waddr = b8(C_addr >> 2);
      C_wdata = C_writeback;
      C_wren  = 1;
    } else if (C_active) {
      // Vane C writes _result to _rd if the thread is active and _rd != 0.
      C_waddr = cat(C_hart, b5(C_insn.r.rd));
      C_wdata = C_writeback;
      C_wren  = C_insn.r.rd && C_insn.r.op != RV32I::OP_STORE && C_insn.r.op != RV32I::OP_BRANCH;
    }
    else {
      C_waddr = b13(DONTCARE);
      C_wdata = b32(DONTCARE);
      C_wren  = 0;
    }

    reg_if.waddr = C_waddr;
    reg_if.wdata = C_wdata;
    reg_if.wren  = C_wren;

    //--------------------------------------------------------------------------
    // Regfile read

    // If vane A is idle, vane B uses vane A's regfile slot to do an additional
    // regfile read. This is used for memory-mapped regfile reading.

    logic<13> A_raddr1;
    logic<13> A_raddr2;

    if (A_active) {
      A_raddr1 = cat(A_hart, b5(A_insn.r.rs1));
      A_raddr2 = cat(A_hart, b5(A_insn.r.rs2));
    }
    else if (B_read_regfile) {
      A_raddr1 = b13(DONTCARE);
      A_raddr2 = b13(B_addr >> 2);
    }
    else {
      A_raddr1 = b13(DONTCARE);
      A_raddr2 = b13(DONTCARE);
    }

    reg_if.raddr1 = A_raddr1;
    reg_if.raddr2 = A_raddr2;

    //--------------------------------------------------------------------------
    // Code bus read/write

    // If the other thread is idle, vane C can override the code bus read to
    // write to code memory.
    // We can _not_ read code memory here as the read would come back too late
    // to write it to the regfile.

    if (C_write_code && !A_active_next) {
      code_tla = gen_bus(C_insn.r.op, C_insn.r.f3, C_addr, C_result);
    }
    else {
      code_tla = gen_bus(RV32I::OP_LOAD, 2, b32(b24(A_hpc_next)), 0);
    }

    data_tla = gen_bus(B_insn.r.op, B_insn.r.f3, B_addr, B_reg2);

    //----------

    tick(reset_in, A_hpc_next, B_addr, B_result, C_waddr, C_wdata, C_wren);
  }

  //----------------------------------------------------------------------------

private:

  //----------------------------------------------------------------------------

  static logic<32> execute_system(logic<8> hart, rv32_insn insn, logic<32> B_reg1) {
    // FIXME need a good error if case is missing an expression
    switch(insn.r.f3) {
      case RV32I::F3_CSRRW: {
        if (insn.c.csr == RV32I::MHARTID) return b32(hart);
        return b32(DONTCARE);
      }
      case RV32I::F3_CSRRS: {
        if (insn.c.csr == RV32I::MHARTID) return b32(hart);
        return b32(DONTCARE);
      }
      case RV32I::F3_CSRRC:  return b32(DONTCARE);
      case RV32I::F3_CSRRWI: return b32(DONTCARE);
      case RV32I::F3_CSRRSI: return b32(DONTCARE);
      case RV32I::F3_CSRRCI: return b32(DONTCARE);
      default:               return b32(DONTCARE);
    }
  }

  //----------------------------------------------------------------------------

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
      default:             return b1(DONTCARE);
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
    switch (insn.r.f3) {
      case RV32I::F3_ADDSUB: return (insn.r.op == RV32I::OP_OP && insn.r.f7 == 32) ? alu1 - alu2 : alu1 + alu2;
      case RV32I::F3_SL:     return alu1 << b5(alu2);
      case RV32I::F3_SLT:    return signed(alu1) < signed(alu2);
      case RV32I::F3_SLTU:   return alu1 < alu2;
      case RV32I::F3_XOR:    return alu1 ^ alu2;
      case RV32I::F3_SR:     return insn.r.f7 == 32 ? signed(alu1) >> b5(alu2) : alu1 >> b5(alu2);
      case RV32I::F3_OR:     return alu1 | alu2;
      case RV32I::F3_AND:    return alu1 & alu2;
      default:               return b32(DONTCARE);
    }
  }

  //----------------------------------------------------------------------------

  void tick(logic<1> reset_in, logic<32> A_hpc_next, logic<32> B_addr, logic<32> B_result, logic<13> C_waddr, logic<32> C_wdata, logic<1> C_wren)
  {
    if (reset_in) {
      A_hpc = 0x00000004;
      B_hpc = 0x00000000;
      C_hpc = 0x00000000;
      D_hpc = 0x00000000;
      ticks = 0x00000000;
    }
    else {
      D_hpc   = C_hpc;
      D_waddr = C_waddr;
      D_wdata = C_wdata;
      D_wren  = C_wren;

      C_hpc    = B_hpc;
      C_insn   = B_insn;
      C_addr   = B_addr;
      C_result = B_result;

      B_hpc    = A_hpc;
      B_insn   = A_insn;

      A_hpc    = A_hpc_next;

      ticks    = ticks + 1;
    }
  }

public:

  //----------------------------------------
  // Internal signals and registers

  /* metron_internal */ logic<32> A_hpc;
  /* metron_internal */ rv32_insn A_insn;

  /* metron_internal */ logic<32> B_hpc;
  /* metron_internal */ rv32_insn B_insn;

  /* metron_internal */ logic<32> C_hpc;
  /* metron_internal */ rv32_insn C_insn;
  /* metron_internal */ logic<32> C_addr;
  /* metron_internal */ logic<32> C_result;

  /* metron_internal */ logic<32> D_hpc;
  /* metron_internal */ logic<13> D_waddr;
  /* metron_internal */ logic<32> D_wdata;
  /* metron_internal */ logic<1>  D_wren;

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
