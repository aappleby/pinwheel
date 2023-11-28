#ifndef PINWHEEL_RTL_PINWHEEL_CORE_H
#define PINWHEEL_RTL_PINWHEEL_CORE_H

#include "metron/metron_tools.h"
#include "pinwheel/tools/regfile_if.h"
#include "pinwheel/tools/tilelink.h"
#include "pinwheel/tools/riscv_constants.h"

#include <assert.h>

// CSR 0x800 - swap secondary thread
// CSR 0x801 - yield primary thread

// 0xE0000000 - mapped regfile base address

// AB - Read registers
// BC - Execute n stuff
// CD - Memory read/write (todo)
// DA - Register writeback

// TODO - Move mem ops to CD so we can read code and still make the writeback

//------------------------------------------------------------------------------
/* verilator lint_off UNUSEDSIGNAL */

class pinwheel_core {
public:

  pinwheel_core() {
    A_hpc = 0;

    B_hpc = 0;
    B_insn.raw = 0;

    C_hpc = 0;
    C_insn.raw = 0;
    C_addr = 0;
    C_result = 0;

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

    logic<24> A_pc = b24(A_hpc);
    logic<24> B_pc = b24(B_hpc);
    logic<24> C_pc = b24(C_hpc);

    logic<1> A_active = A_pc != 0;
    logic<1> B_active = B_pc != 0;
    logic<1> C_active = C_pc != 0;

    rv32_insn AB_insn;
    AB_insn.raw = A_active ? code_tld.d_data : b32(0);

    //----------------------------------------
    // Forward stores from phase D to phase B

    if (D_waddr == cat(B_hart, b5(B_insn.r.rs1)) && D_wren) reg1 = D_wdata;
    if (D_waddr == cat(B_hart, b5(B_insn.r.rs2)) && D_wren) reg2 = D_wdata;

    logic<32> BC_reg1 = B_insn.r.rs1 ? reg1 : b32(0);
    logic<32> BC_reg2 = B_insn.r.rs2 ? reg2 : b32(0);
    logic<32> BC_imm  = decode_imm(B_insn);
    logic<32> BC_addr = b32(BC_reg1 + BC_imm);

    //----------------------------------------

    logic<1> BC_yield = B_active && B_insn.r.op == RV32I::OP_SYSTEM && B_insn.r.f3 == RV32I::F3_CSRRW && B_insn.c.csr == 0x801;
    logic<1> CD_yield = C_active && C_insn.r.op == RV32I::OP_SYSTEM && C_insn.r.f3 == RV32I::F3_CSRRW && C_insn.c.csr == 0x801;

    logic<1> BC_swap  = B_active && B_insn.r.op == RV32I::OP_SYSTEM && B_insn.r.f3 == RV32I::F3_CSRRW && B_insn.c.csr == 0x800;
    logic<1> CD_swap  = C_active && C_insn.r.op == RV32I::OP_SYSTEM && C_insn.r.f3 == RV32I::F3_CSRRW && C_insn.c.csr == 0x800;

    logic<1> BC_read_regfile  = B_active && B_insn.r.op == RV32I::OP_LOAD  && b4(BC_addr, 28) == 0xE;

    logic<1> CD_read_code     = C_active && C_insn.r.op == RV32I::OP_LOAD  && b4(C_addr, 28) == 0x0;
    logic<1> CD_read_regfile  = C_active && C_insn.r.op == RV32I::OP_LOAD  && b4(C_addr, 28) == 0xE;
    logic<1> CD_read_mem      = C_active && C_insn.r.op == RV32I::OP_LOAD  && b4(C_addr, 28) == 0x8;

    logic<1> CD_write_code    = C_active && C_insn.r.op == RV32I::OP_STORE && b4(C_addr, 28) == 0x0;
    logic<1> CD_write_mem     = C_active && C_insn.r.op == RV32I::OP_STORE && b4(C_addr, 28) == 0x8;
    logic<1> CD_write_regfile = C_active && C_insn.r.op == RV32I::OP_STORE && b4(C_addr, 28) == 0xE;

    //----------------------------------------

    logic<1>  BC_take_branch = take_branch(B_insn.r.f3, BC_reg1, BC_reg2);
    logic<32> BC_pc_jump = B_pc + BC_imm;
    logic<32> BC_pc_next = B_pc + 4;
    logic<24> BC_next_pc;

    switch(B_insn.r.op) {
      case RV32I::OP_BRANCH: BC_next_pc = BC_take_branch ? BC_pc_jump : BC_pc_next; break;
      case RV32I::OP_JAL:    BC_next_pc = BC_pc_jump; break;
      case RV32I::OP_JALR:   BC_next_pc = BC_addr; break;
      case RV32I::OP_LUI:    BC_next_pc = BC_pc_next; break;
      case RV32I::OP_AUIPC:  BC_next_pc = BC_pc_next; break;
      case RV32I::OP_LOAD:   BC_next_pc = BC_pc_next; break;
      case RV32I::OP_STORE:  BC_next_pc = BC_pc_next; break;
      case RV32I::OP_SYSTEM: BC_next_pc = BC_pc_next; break;
      case RV32I::OP_OPIMM:  BC_next_pc = BC_pc_next; break;
      case RV32I::OP_OP:     BC_next_pc = BC_pc_next; break;
      default:               BC_next_pc = b24(DONTCARE); break;
    }

    logic<32> BC_hpc = cat(B_hart, BC_next_pc);

    //----------------------------------------
    // Vane B executes its instruction and stores the result in _result.

    logic<32> BC_result;
    switch(B_insn.r.op) {
      case RV32I::OP_OPIMM:  BC_result = execute_alu(B_insn, BC_reg1, BC_imm); break;
      case RV32I::OP_OP:     BC_result = execute_alu(B_insn, BC_reg1, BC_reg2); break;
      case RV32I::OP_SYSTEM: BC_result = execute_system(B_hart, B_insn, BC_reg1); break;
      case RV32I::OP_BRANCH: BC_result = b32(DONTCARE); break;
      case RV32I::OP_JAL:    BC_result = BC_pc_next; break;
      case RV32I::OP_JALR:   BC_result = BC_pc_next; break;
      case RV32I::OP_LUI:    BC_result = BC_imm; break;
      case RV32I::OP_AUIPC:  BC_result = B_pc + BC_imm; break;
      case RV32I::OP_LOAD:   BC_result = b32(DONTCARE); break;
      case RV32I::OP_STORE:  BC_result = BC_reg2; break;
      default:               BC_result = b32(DONTCARE); break;
    }

    // If this thread is yielding, we store where it _would've_ gone in B_result
    // so we can write it to the regfile in phase C.
    if (BC_yield) BC_result = BC_hpc;

    // If we're going to swap secondary threads in phase C, our target hpc is
    // coming from reg1. We need to stash it in B_result so we can use it next
    // phase from C_result below.
    if (BC_swap)  BC_result = BC_reg1;

    //--------------------------------------------------------------------------
    // If both threads trigger PC swaps at once, the C one fires first.

    logic<32> BA_hpc;

    if (CD_swap) {
      BA_hpc = C_result;
    }
    else if (BC_yield) {
      BA_hpc = BC_reg1;
    }
    else {
      BA_hpc = BC_hpc;
    }

    logic<1> BA_active = b24(BA_hpc) != 0;

    //--------------------------------------------------------------------------

    logic<32> CD_mem = unpack_mem(C_insn.r.f3, C_addr, data_tld.d_data);

    //--------------------------------------------------------------------------

    logic<32> CD_writeback;

    if (CD_yield) {
      CD_writeback = C_result;
    }
    else if (CD_swap) {
      // If we're switching secondary threads, we write the previous secondary
      // thread back to the primary thread's regfile.
      CD_writeback = BC_hpc;
    }
    else if (CD_read_regfile) {
      // Note - this must be the _raw_ register, not zeroed, if we want to use
      // R0s in the regfile as spare storage
      CD_writeback = reg2;
    }
    else if (CD_read_mem) {
      // A memory read replaces _result with the unpacked value on the data bus.
      CD_writeback = CD_mem;
    }
    else if (C_active) {
      CD_writeback = C_result;
    }
    else {
      CD_writeback = b32(DONTCARE);
    }

    //--------------------------------------------------------------------------
    // Regfile write

    logic<13> CD_waddr;
    logic<32> CD_wdata;
    logic<1>  CD_wren;

    if (CD_write_regfile) {
      // A memory-mapped regfile write overrides the normal write.
      CD_waddr = b8(C_addr >> 2);
      CD_wdata = CD_writeback;
      CD_wren  = 1;
    } else if (C_active) {
      // Vane C writes _result to _rd if the thread is active and _rd != 0.
      CD_waddr = cat(C_hart, b5(C_insn.r.rd));
      CD_wdata = CD_writeback;
      CD_wren  = C_insn.r.rd && C_insn.r.op != RV32I::OP_STORE && C_insn.r.op != RV32I::OP_BRANCH;
    }
    else {
      CD_waddr = b13(DONTCARE);
      CD_wdata = b32(DONTCARE);
      CD_wren  = 0;
    }

    //--------------------------------------------------------------------------
    // Regfile read

    // If vane A is idle, vane B uses vane A's regfile slot to do an additional
    // regfile read. This is used for memory-mapped regfile reading.

    logic<13> AB_raddr1;
    logic<13> AB_raddr2;

    if (A_active) {
      AB_raddr1 = cat(A_hart, b5(AB_insn.r.rs1));
      AB_raddr2 = cat(A_hart, b5(AB_insn.r.rs2));
    }
    else if (BC_read_regfile) {
      AB_raddr1 = b13(DONTCARE);
      AB_raddr2 = b13(BC_addr >> 2);
    }
    else {
      AB_raddr1 = b13(DONTCARE);
      AB_raddr2 = b13(DONTCARE);
    }

    //--------------------------------------------------------------------------
    // Code bus read/write

    // If the other thread is idle, vane C can override the code bus read to
    // write to code memory.

    if ((CD_read_code || CD_write_code) && !BA_active) {
      code_tla = gen_bus(C_insn.r.op, C_insn.r.f3, C_addr, C_result);
    }
    else {
      code_tla = gen_bus(RV32I::OP_LOAD, 2, b32(b24(BA_hpc)), 0);
    }

    // FIXME move to phase C
    data_tla = gen_bus(B_insn.r.op, B_insn.r.f3, BC_addr, BC_reg2);

    reg_if.waddr  = D_waddr;
    reg_if.wdata  = D_wdata;
    reg_if.wren   = D_wren;
    reg_if.raddr1 = AB_raddr1;
    reg_if.raddr2 = AB_raddr2;

    //----------

    tick(reset_in, AB_insn, BA_hpc, BC_addr, BC_result, CD_waddr, CD_wdata, CD_wren);
  }

  //----------------------------------------------------------------------------

private:

  //----------------------------------------------------------------------------

  void tick(logic<1> reset_in,
            rv32_insn AB_insn,
            logic<32> BA_hpc,
            logic<32> BC_addr, logic<32> BC_result,
            logic<13> CD_waddr, logic<32> CD_wdata, logic<1> CD_wren)
  {
    if (reset_in) {
      A_hpc      = 0x00000004;

      B_hpc      = 0x00000000;
      B_insn.raw = 0;

      C_hpc      = 0;
      C_insn.raw = 0;
      C_addr     = 0;
      C_result   = 0;

      D_waddr = 0;
      D_wdata = 0;
      D_wren  = 0;

      ticks = 0x00000000;
    }
    else {
      D_waddr  = CD_waddr;
      D_wdata  = CD_wdata;
      D_wren   = CD_wren;

      C_hpc    = B_hpc;
      C_insn   = B_insn;
      C_addr   = BC_addr;
      C_result = BC_result;

      B_hpc    = A_hpc;
      B_insn   = AB_insn;

      A_hpc    = BA_hpc;

      ticks    = ticks + 1;
    }
  }


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
  // Translates a RV32I opcode into a TilelinkA transaction

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

public:

  //----------------------------------------
  // Internal signals and registers

  /* metron_internal */ logic<32> A_hpc;

  /* metron_internal */ logic<32> B_hpc;
  /* metron_internal */ rv32_insn B_insn;

  /* metron_internal */ logic<32> C_hpc;
  /* metron_internal */ rv32_insn C_insn;
  /* metron_internal */ logic<32> C_addr;
  /* metron_internal */ logic<32> C_result;

  /* metron_internal */ logic<13> D_waddr;
  /* metron_internal */ logic<32> D_wdata;
  /* metron_internal */ logic<1>  D_wren;

  /* metron_internal */ logic<32> ticks;

};

/* verilator lint_on UNUSEDSIGNAL */
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_PINWHEEL_CORE_H
