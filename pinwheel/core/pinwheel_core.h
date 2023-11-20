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
    A_pc = 0;
    //A_insn = 0; // signal
    A_insn2.raw = 0;
    B_pc = 0;
    //B_insn = 0;
    B_insn2.raw = 0;
    C_pc = 0;
    C_insn2.raw = 0;
    C_addr = 0;
    C_result = 0;
    D_pc = 0;
    D_insn2.raw = 0;
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

  /* metron_noconvert */ logic<32> dbg_decode_imm(logic<32> insn) const { return decode_imm(insn); }

  //----------------------------------------

  // FIXME yosys does not like this as a local variable
  tilelink_a data_tla;
  tilelink_a code_tla;
  regfile_if reg_if;

  void tock_data_bus(logic<32> reg_rdata1, logic<32> reg_rdata2) {

    //----------
    // Decode instruction B

    logic<32> B_reg1 = B_insn2.r.rs1 ? reg_rdata1 : b32(0);
    logic<32> B_reg2 = B_insn2.r.rs2 ? reg_rdata2 : b32(0);
    logic<32> B_imm  = decode_imm2(B_insn2);
    logic<32> B_addr = b32(B_reg1 + B_imm);

    //----------
    // Data bus read/write

    logic<3>  bus_size = b3(DONTCARE);
    logic<4>  mask_b   = 0;

    if (B_insn2.r.f3 == 0)    { mask_b = 0b0001; bus_size = 0; }
    if (B_insn2.r.f3 == 1)    { mask_b = 0b0011; bus_size = 1; }
    if (B_insn2.r.f3 == 2)    { mask_b = 0b1111; bus_size = 2; }
    if (B_addr[0]) mask_b = mask_b << 1;
    if (B_addr[1]) mask_b = mask_b << 2;

    data_tla.a_address = B_addr;
    data_tla.a_data    = (B_reg2 << ((B_addr & 3) * 8));
    data_tla.a_mask    = mask_b;
    data_tla.a_opcode  = (B_insn2.r.op == RV32I::OP2_STORE) ? (bus_size == 2 ? TL::PutFullData : TL::PutPartialData) : TL::Get;
    data_tla.a_param   = b3(DONTCARE);
    data_tla.a_size    = bus_size;
    data_tla.a_source  = b1(DONTCARE);
    data_tla.a_valid   = (B_insn2.r.op == RV32I::OP2_LOAD) || (B_insn2.r.op == RV32I::OP2_STORE);
    data_tla.a_ready   = 1;
  }

  //----------------------------------------

  /* metron_internal */
  logic<32> execute(logic<32> reg1, logic<32> reg2) const {
    logic<32> result = 0;

    logic<3>  B_f3   = B_insn2.r.f3;
    logic<5>  B_rs1  = B_insn2.r.rs1;
    logic<5>  B_rs2  = B_insn2.r.rs2;
    logic<32> B_reg1 = B_rs1 ? reg1 : b32(0);
    logic<32> B_reg2 = B_rs2 ? reg2 : b32(0);
    logic<32> B_imm  = decode_imm2(B_insn2);
    logic<32> B_addr = b32(B_reg1 + B_imm);

    switch(B_insn2.r.op) {
      case RV32I::OP2_OPIMM:  result = execute_alu   (B_insn2.raw, B_reg1, B_reg2); break;
      case RV32I::OP2_OP:     result = execute_alu   (B_insn2.raw, B_reg1, B_reg2); break;
      case RV32I::OP2_SYSTEM: result = execute_system(B_insn2.raw, B_reg1, B_reg2); break;
      case RV32I::OP2_BRANCH: result = b32(DONTCARE); break;
      case RV32I::OP2_JAL:    result = B_pc + 4;      break;
      case RV32I::OP2_JALR:   result = B_pc + 4;      break;
      case RV32I::OP2_LUI:    result = B_imm;         break;
      case RV32I::OP2_AUIPC:  result = B_pc + B_imm;  break;
      case RV32I::OP2_LOAD:   result = B_addr;        break;
      case RV32I::OP2_STORE:  result = B_reg2;        break;
      default:                result = b32(DONTCARE); break;
    }

    return result;
  }

  //----------------------------------------

  void tock(logic<1> reset_in, tilelink_d code_tld, tilelink_d data_tld, logic<32> reg1, logic<32> reg2) {

    //----------
    // Decode instruction A

    //A_insn = b24(A_pc) ? code_tld.d_data : b32(0);
    A_insn2.raw = b24(A_pc) ? code_tld.d_data : b32(0);
    logic<5>  A_rs1  = A_insn2.r.rs1;
    logic<5>  A_rs2  = A_insn2.r.rs2;

    //----------
    // Decode instruction B

    logic<7>  B_op   = B_insn2.r.op;
    logic<3>  B_f3   = B_insn2.r.f3;
    logic<5>  B_rs1  = B_insn2.r.rs1;
    logic<5>  B_rs2  = B_insn2.r.rs2;
    logic<32> B_reg1 = B_rs1 ? reg1 : b32(0);
    logic<32> B_reg2 = B_rs2 ? reg2 : b32(0);
    logic<32> B_imm  = decode_imm2(B_insn2);
    logic<32> B_addr = b32(B_reg1 + B_imm);

    //----------
    // Decode instruction C

    logic<7>  C_op = C_insn2.r.op;
    logic<5>  C_rd = C_insn2.r.rd;
    logic<3>  C_f3 = C_insn2.r.f3;

    //----------
    // Execute

    logic<32> B_result = execute(reg1, reg2);

    //----------
    // Next instruction selection

    logic<32> A_pc_next = next_pc(reg1, reg2);

    //----------
    // PC hackery to swap threads

    logic<32> D_result = C_result;

    // FIXME what if both threads trigger these two PC swaps at once?

    // If we write to CSR 0x800, we swap the secondary thread's PC with the
    // register value.
    logic<12> C_csr = C_insn2.c.csr;
    if (C_op == RV32I::OP2_SYSTEM && C_f3 == RV32I::F3_CSRRW && C_csr == 0x800) {
      D_result = A_pc_next;
      A_pc_next = C_result;
    }

    // If we write to CSR 0x801, we swap the current thread's PC with the
    // register value.
    logic<12> B_csr  = B_insn2.c.csr;
    if (B_op == RV32I::OP2_SYSTEM && B_f3 == RV32I::F3_CSRRW && B_csr == 0x801) {
      logic<32> temp = B_result;
      B_result = A_pc_next;
      A_pc_next = temp;
    }

    //----------
    // Code bus read/write

    {
      // We can write code memory in phase C if the other thread is idle.
      logic<4> C_bus_tag = b4(C_addr, 28);
      logic<1> C_code_cs = C_bus_tag == 0x0 && b24(A_pc_next) == 0;

      logic<4> mask = 0;
      if (C_f3 == 0) mask = 0b0001;
      if (C_f3 == 1) mask = 0b0011;
      if (C_f3 == 2) mask = 0b1111;
      if (C_addr[0]) mask = mask << 1;
      if (C_addr[1]) mask = mask << 2;

      code_tla.a_address = C_code_cs ? b24(C_addr) : b24(A_pc_next);
      code_tla.a_data    = D_result;
      code_tla.a_mask    = mask;
      code_tla.a_opcode  = (C_op == RV32I::OP2_STORE) && C_code_cs ? TL::PutFullData : TL::Get;
      code_tla.a_param   = b3(DONTCARE);
      code_tla.a_size    = 2;
      code_tla.a_source  = b1(DONTCARE);
      code_tla.a_valid   = 1;
      code_tla.a_ready   = 1;
    }

    register_interface(data_tld, reg1, reg2, D_result);

    //----------
    // Writeback to core regs

    tick(reset_in, code_tld.d_data, reg1, A_pc_next, B_result);

    // FIXME - Verilator bug?
    //If I remove this, Verilator complains about bogus latches inferred
    switch(B_op) {
      case 0: break;
    }
  }

  //----------------------------------------

  /* metron_internal */
  void register_interface(tilelink_d data_tld, logic<32> reg1, logic<32> reg2, logic<32> D_result) {

    //----------
    // Decode instruction A

    logic<5>  A_rs1  = A_insn2.r.rs1;
    logic<5>  A_rs2  = A_insn2.r.rs2;

    //----------
    // Decode instruction B

    logic<3>  B_f3   = B_insn2.r.f3;
    logic<5>  B_rs1  = B_insn2.r.rs1;
    logic<5>  B_rs2  = B_insn2.r.rs2;
    logic<32> B_reg1 = B_rs1 ? reg1 : b32(0);
    logic<32> B_reg2 = B_rs2 ? reg2 : b32(0);
    logic<32> B_imm  = decode_imm2(B_insn2);
    logic<32> B_addr = b32(B_reg1 + B_imm);

    //----------
    // Decode instruction C

    logic<7>  C_op = C_insn2.r.op;
    logic<5>  C_rd = C_insn2.r.rd;
    logic<3>  C_f3 = C_insn2.r.f3;

    //----------
    // Regfile read/write

    {
      // By default, we use the register indices from instruction A to read the
      // regfile.
      reg_if.raddr1 = cat(b3(A_pc, 24), A_rs1);
      reg_if.raddr2 = cat(b3(A_pc, 24), A_rs2);

      // But if the other thread is idle, we can read the regfile through the
      // memory mapping.
      logic<1> B_regfile_cs = b4(B_addr, 28) == 0xE;
      if ((B_insn2.r.op == RV32I::OP2_LOAD) && B_regfile_cs && (b24(A_pc) == 0)) {
        reg_if.raddr1 = b10(B_addr >> 2);
      }
    }

    //----------
    // Regfile write

    // Normally we write 'result' to 'rd' if rd != 0 and the thread is active.
    reg_if.waddr = cat(b3(C_pc, 24), C_rd);
    reg_if.wdata = D_result;
    reg_if.wren  = C_rd && b24(C_pc);

    logic<1> C_regfile_cs = b4(C_addr, 28) == 0xE;

    // Loads write the contents of the memory bus to the regfile.
    if (C_op == RV32I::OP2_LOAD) {
      // The result of the last memory read comes from either the data bus,
      // or the regfile if we read from the memory-mapped regfile last tock.
      logic<32> C_mem = C_regfile_cs ? reg1 : data_tld.d_data;

      if (D_result[0]) C_mem = C_mem >> 8;
      if (D_result[1]) C_mem = C_mem >> 16;
      switch (C_f3) {
        case 0:  C_mem = sign_extend<32>( b8(C_mem)); break;
        case 1:  C_mem = sign_extend<32>(b16(C_mem)); break;
        case 4:  C_mem = zero_extend<32>( b8(C_mem)); break;
        case 5:  C_mem = zero_extend<32>(b16(C_mem)); break;
      }

      reg_if.waddr = cat(b3(C_pc, 24), C_rd);
      reg_if.wdata = C_mem;
      reg_if.wren  = C_rd && b24(C_pc);
    }

    // Stores don't write to the regfile unless there's a memory-mapped write
    // to the regfile.
    else if (C_op == RV32I::OP2_STORE) {
      if (C_regfile_cs) {
        reg_if.waddr = b10(C_addr >> 2);
        reg_if.wdata = D_result;
        reg_if.wren  = 1;
      }
      else {
        reg_if.waddr = b10(DONTCARE);
        reg_if.wdata = b32(DONTCARE);
        reg_if.wren  = 0;
      }
    }

    // Branches never write to the regfile.
    else if (C_op == RV32I::OP2_BRANCH) {
      reg_if.waddr = b10(DONTCARE);
      reg_if.wdata = b32(DONTCARE);
      reg_if.wren  = 0;
    }

    // If we're using jalr to jump between threads, we use the hart from pc _A_
    // as the target for the write so that the link register will be written
    // in the _destination_ regfile.
    else if (C_op == RV32I::OP2_JALR) {
      reg_if.waddr = cat(b3(A_pc, 24), C_rd);
    }
  }


  //----------------------------------------

private:

  //----------------------------------------

  void tick(logic<1> reset_in,
            logic<32> code_tld_data,
            logic<32> reg_rdata1,
            logic<32> A_pc_next,
            logic<32> B_result)
  {
    logic<5>  B_rs1  = B_insn2.r.rs1;
    logic<32> B_reg1 = B_rs1 ? reg_rdata1 : b32(0);
    logic<32> B_imm  = decode_imm2(B_insn2);
    logic<32> B_addr = b32(B_reg1 + B_imm);

    if (reset_in) {
      A_pc     = 0x00400000;

      B_pc     = 0;
      //B_insn   = 0;
      B_insn2.raw = 0;

      C_pc     = 0;
      C_insn2.raw   = 0;
      C_addr   = 0;
      C_result = 0;

      D_pc     = 0;
      D_insn2.raw   = 0;

      ticks    = 0;
    }
    else {
      D_pc     = C_pc;
      D_insn2.raw   = C_insn2.raw;

      C_pc     = B_pc;
      C_insn2.raw   = B_insn2.raw;
      C_addr   = B_addr;
      C_result = B_result;

      B_pc     = A_pc;
      //B_insn   = A_insn;
      B_insn2.raw = A_insn2.raw;

      A_pc     = A_pc_next;

      ticks    = ticks + 1;
    }
  }

  //----------------------------------------

  logic<32> decode_imm(logic<32> insn) const {
    logic<7>  op    = b7(insn);
    logic<32> imm_i = sign_extend<32>(b12(insn, 20));
    logic<32> imm_s = cat(dup<21>(insn[31]), b6(insn, 25), b5(insn, 7));
    logic<32> imm_u = b20(insn, 12) << 12;
    logic<32> imm_b = cat(dup<20>(insn[31]), insn[7], b6(insn, 25), b4(insn, 8), b1(0));
    logic<32> imm_j = cat(dup<12>(insn[31]), b8(insn, 12), insn[20], b10(insn, 21), b1(0));

    logic<32> result;
    switch(op) {
      case RV32I::OP2_LOAD:   result = imm_i; break;
      case RV32I::OP2_OPIMM:  result = imm_i; break;
      case RV32I::OP2_AUIPC:  result = imm_u; break;
      case RV32I::OP2_STORE:  result = imm_s; break;
      case RV32I::OP2_OP:     result = imm_i; break;
      case RV32I::OP2_LUI:    result = imm_u; break;
      case RV32I::OP2_BRANCH: result = imm_b; break;
      case RV32I::OP2_JALR:   result = imm_i; break;
      case RV32I::OP2_JAL:    result = imm_j; break;
      default:               result = 0;     break;
    }
    return result;
  }

  logic<32> decode_imm2(rv32_insn insn2) const {
    logic<32> insn  = insn2.raw;
    logic<7>  op    = b7(insn);
    logic<32> imm_i = sign_extend<32>(b12(insn, 20));
    logic<32> imm_s = cat(dup<21>(insn[31]), b6(insn, 25), b5(insn, 7));
    logic<32> imm_u = b20(insn, 12) << 12;
    logic<32> imm_b = cat(dup<20>(insn[31]), insn[7], b6(insn, 25), b4(insn, 8), b1(0));
    logic<32> imm_j = cat(dup<12>(insn[31]), b8(insn, 12), insn[20], b10(insn, 21), b1(0));

    logic<32> result;
    switch(op) {
      case RV32I::OP2_LOAD:   result = imm_i; break;
      case RV32I::OP2_OPIMM:  result = imm_i; break;
      case RV32I::OP2_AUIPC:  result = imm_u; break;
      case RV32I::OP2_STORE:  result = imm_s; break;
      case RV32I::OP2_OP:     result = imm_i; break;
      case RV32I::OP2_LUI:    result = imm_u; break;
      case RV32I::OP2_BRANCH: result = imm_b; break;
      case RV32I::OP2_JALR:   result = imm_i; break;
      case RV32I::OP2_JAL:    result = imm_j; break;
      default:               result = 0;     break;
    }
    return result;
  }

  //----------------------------------------

  logic<32> execute_alu(logic<32> insn, logic<32> reg1, logic<32> reg2) const {
    logic<7>  op  = b7(insn);
    logic<3>  f3  = b3(insn, 12);
    logic<7>  f7  = b7(insn, 25);
    logic<32> imm = decode_imm(insn);

    logic<32> alu1 = reg1;
    logic<32> alu2 = op == RV32I::OP2_OPIMM ? imm : reg2;
    if (op == RV32I::OP2_OP && f3 == 0 && f7 == 32) alu2 = -alu2;

    logic<32> result;
    switch (f3) {
      case 0:  result = alu1 + alu2; break;
      case 1:  result = alu1 << b5(alu2); break;
      case 2:  result = signed(alu1) < signed(alu2); break;
      case 3:  result = alu1 < alu2; break;
      case 4:  result = alu1 ^ alu2; break;
      case 5:  result = f7 == 32 ? signed(alu1) >> b5(alu2) : alu1 >> b5(alu2); break;
      case 6:  result = alu1 | alu2; break;
      case 7:  result = alu1 & alu2; break;
      default: result = 0; break;
    }
    return result;
  }

  //----------------------------------------

  logic<32> execute_system(logic<32> insn, logic<32> reg1, logic<32> reg2) const {
    logic<3>  f3  = b3(insn, 12);
    logic<12> csr = b12(insn, 20);

    // FIXME need a good error if case is missing an expression
    logic<32> result = 0;
    switch(f3) {
      case RV32I::F3_CSRRW: {
        result = reg1;
        break;
      }
      case RV32I::F3_CSRRS: {
        if (csr == 0xF14) result = b8(B_pc, 24);
        break;
      }
      case RV32I::F3_CSRRC:  result = 0; break;
      case RV32I::F3_CSRRWI: result = 0; break;
      case RV32I::F3_CSRRSI: result = 0; break;
      case RV32I::F3_CSRRCI: result = 0; break;
    }
    return result;
  }

  //----------------------------------------

  logic<32> next_pc(logic<32> reg1, logic<32> reg2) const {

    logic<32> pc = 0;

    if (b24(B_pc)) {
      logic<7>  B_op   = B_insn2.r.op;
      logic<3>  B_f3   = B_insn2.r.f3;
      logic<5>  B_rs1  = B_insn2.r.rs1;
      logic<5>  B_rs2  = B_insn2.r.rs2;
      logic<32> B_reg1 = B_rs1 ? reg1 : b32(0);
      logic<32> B_reg2 = B_rs2 ? reg2 : b32(0);
      logic<32> B_imm  = decode_imm2(B_insn2);
      logic<32> B_addr = b32(B_reg1 + B_imm);

      logic<1> eq  = B_reg1 == B_reg2;
      logic<1> slt = signed(B_reg1) < signed(B_reg2);
      logic<1> ult = B_reg1 < B_reg2;
      logic<1> take_branch = 0;

      switch (B_f3) {
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

      switch(B_op) {
        case RV32I::OP2_BRANCH: pc = take_branch ? B_pc + B_imm : B_pc + 4; break;
        case RV32I::OP2_JAL:    pc = B_pc + B_imm; break;
        case RV32I::OP2_JALR:   pc = B_addr; break;
        case RV32I::OP2_LUI:    pc = B_pc + 4; break;
        case RV32I::OP2_AUIPC:  pc = B_pc + 4; break;
        case RV32I::OP2_LOAD:   pc = B_pc + 4; break;
        case RV32I::OP2_STORE:  pc = B_pc + 4; break;
        case RV32I::OP2_SYSTEM: pc = B_pc + 4; break;
        case RV32I::OP2_OPIMM:  pc = B_pc + 4; break;
        case RV32I::OP2_OP:     pc = B_pc + 4; break;
      }
    }

    // Patch in the hart index from B's PC into the _next_ A's pc
    pc = (pc & 0x00FFFFFF) | (B_pc & 0xFF000000);
    return pc;
  }

public:

  //----------------------------------------
  // Internal signals and registers
  // metron_internal

  /* metron_internal */ logic<32> A_pc;
  ///* metron_internal */ logic<32> A_insn;
  /* metron_internal */ rv32_insn A_insn2;

  /* metron_internal */ logic<32> B_pc;
  ///* metron_internal */ logic<32> B_insn;
  /* metron_internal */ rv32_insn B_insn2;

  /* metron_internal */ logic<32> C_pc;
  /* metron_internal */ rv32_insn C_insn2;
  /* metron_internal */ logic<32> C_addr;
  /* metron_internal */ logic<32> C_result;

  // These two registers aren't actually needed, but they make debugging easier.
  /* metron_internal */ logic<32> D_pc;
  /* metron_internal */ rv32_insn D_insn2;

  /* metron_internal */ logic<32> ticks;

};

/* verilator lint_on UNUSEDSIGNAL */
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_PINWHEEL_CORE_H
