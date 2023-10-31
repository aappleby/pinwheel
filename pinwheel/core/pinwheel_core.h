#ifndef PINWHEEL_RTL_PINWHEEL_CORE_H
#define PINWHEEL_RTL_PINWHEEL_CORE_H

#include "metron/metron_tools.h"
#include "pinwheel/tools/regfile_if.h"
#include "pinwheel/tools/tilelink.h"
#include "pinwheel/tools/riscv_constants.h"

//------------------------------------------------------------------------------
/* verilator lint_off UNUSEDSIGNAL */

class pinwheel_core {
public:

  /* metron_noconvert */ logic<32> dbg_decode_imm(logic<32> insn) const { return decode_imm(insn); }

  //----------------------------------------

  void tock(logic<1> reset_in, tilelink_d code_tld, tilelink_d bus_tld, logic<32> reg_rdata1, logic<32> reg_rdata2) {

    {
      logic<32> sig_insn_a  = b24(reg_hpc_a) ? code_tld.d_data : b32(0);
      logic<5>  rs1a_a  = b5(sig_insn_a, 15);
      logic<5>  rs2a_a  = b5(sig_insn_a, 20);
      reg_if.raddr1 = cat(b3(reg_hpc_a, 24), rs1a_a);
      reg_if.raddr2 = cat(b3(reg_hpc_a, 24), rs2a_a);
    }


    //----------
    // Fetch

    logic<32> next_hpc_a = 0;
    {
      logic<1> take_branch = 0;
      if (b24(reg_hpc_b)) {
        logic<5>  rs1a_b = b5(reg_insn_b, 15);
        logic<5>  rs2a_b = b5(reg_insn_b, 20);
        logic<32> rs1_b  = rs1a_b ? reg_rdata1 : b32(0);
        logic<32> rs2_b  = rs2a_b ? reg_rdata2 : b32(0);
        logic<1> eq  = rs1_b == rs2_b;
        logic<1> slt = signed(rs1_b) < signed(rs2_b);
        logic<1> ult = rs1_b < rs2_b;

        logic<3>  f3_b   = b3(reg_insn_b, 12);
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
      }

      if (b24(reg_hpc_b)) {
        logic<5>  rs1a_b = b5(reg_insn_b, 15);
        logic<32> rs1_b  = rs1a_b ? reg_rdata1 : b32(0);
        logic<32> imm_b = decode_imm(reg_insn_b);
        logic<32> sig_addr_b  = b32(rs1_b + imm_b);
        logic<5>  op_b   = b5(reg_insn_b, 2);
        switch(op_b) {
          case RV32I::OP_BRANCH: next_hpc_a = take_branch ? reg_hpc_b + imm_b : reg_hpc_b + 4; break;
          case RV32I::OP_JAL:    next_hpc_a = reg_hpc_b + imm_b; break;
          case RV32I::OP_JALR:   next_hpc_a = sig_addr_b; break;
          case RV32I::OP_LUI:    next_hpc_a = reg_hpc_b + 4; break;
          case RV32I::OP_AUIPC:  next_hpc_a = reg_hpc_b + 4; break;
          case RV32I::OP_LOAD:   next_hpc_a = reg_hpc_b + 4; break;
          case RV32I::OP_STORE:  next_hpc_a = reg_hpc_b + 4; break;
          case RV32I::OP_SYSTEM: next_hpc_a = reg_hpc_b + 4; break;
          case RV32I::OP_OPIMM:  next_hpc_a = reg_hpc_b + 4; break;
          case RV32I::OP_OP:     next_hpc_a = reg_hpc_b + 4; break;
        }
      }
    }

    next_hpc_a = (next_hpc_a & 0x00FFFFFF) | (reg_hpc_b & 0xFF000000);

    //----------
    // Execute

    logic<32> result_c = reg_result_c;
    logic<32> alu_result = 0;
    {
      logic<5>  rs1a_b = b5(reg_insn_b, 15);
      logic<5>  rs2a_b = b5(reg_insn_b, 20);
      logic<32> rs1_b  = rs1a_b ? reg_rdata1 : b32(0);
      logic<32> rs2_b  = rs2a_b ? reg_rdata2 : b32(0);
      logic<32> imm_b = decode_imm(reg_insn_b);
      logic<32> sig_addr_b  = b32(rs1_b + imm_b);
      logic<5>  op_b   = b5(reg_insn_b, 2);
      switch(op_b) {
        case RV32I::OP_BRANCH: alu_result = b32(DONTCARE);     break;
        case RV32I::OP_JAL:    alu_result = reg_hpc_b + 4;     break;
        case RV32I::OP_JALR:   alu_result = reg_hpc_b + 4;     break;
        case RV32I::OP_LUI:    alu_result = imm_b;             break;
        case RV32I::OP_AUIPC:  alu_result = reg_hpc_b + imm_b; break;
        case RV32I::OP_LOAD:   alu_result = sig_addr_b;        break;
        case RV32I::OP_STORE:  alu_result = rs2_b;             break;
        case RV32I::OP_SYSTEM: alu_result = execute_system(reg_insn_b, rs1_b, rs2_b); break;
        case RV32I::OP_OPIMM:  alu_result = execute_alu   (reg_insn_b, rs1_b, rs2_b); break;
        case RV32I::OP_OP:     alu_result = execute_alu   (reg_insn_b, rs1_b, rs2_b); break;
        default:               alu_result = b32(DONTCARE);     break;
      }

      logic<12> csr_b = b12(reg_insn_b, 20);
      logic<3>  f3_b   = b3(reg_insn_b, 12);
      if (op_b == RV32I::OP_SYSTEM && f3_b == RV32I::F3_CSRRW && csr_b == 0x801) {
        logic<32> temp = alu_result;
        alu_result = next_hpc_a;
        next_hpc_a = temp;
      }

      logic<12> csr_c = b12(reg_insn_c, 20);
      logic<3> f3_c = b3(reg_insn_c, 12);
      logic<5> op_c = b5(reg_insn_c, 2);

      if (op_c == RV32I::OP_SYSTEM && f3_c == RV32I::F3_CSRRW && csr_c == 0x800) {
        result_c = next_hpc_a;
        next_hpc_a = reg_result_c;
      }
    }

    //----------
    // Memory: Data bus

    {
      logic<3> bus_size = b3(DONTCARE);
      logic<32> imm_b = decode_imm(reg_insn_b);
      logic<5>  rs1a_b = b5(reg_insn_b, 15);
      logic<32> rs1_b  = rs1a_b ? reg_rdata1 : b32(0);
      logic<32> sig_addr_b  = b32(rs1_b + imm_b);
      logic<4> mask_b = 0;
      logic<5>  rs2a_b = b5(reg_insn_b, 20);
      logic<32> rs2_b  = rs2a_b ? reg_rdata2 : b32(0);
      logic<3>  f3_b   = b3(reg_insn_b, 12);
      if (f3_b == 0)    { mask_b = 0b0001; bus_size = 0; }
      if (f3_b == 1)    { mask_b = 0b0011; bus_size = 1; }
      if (f3_b == 2)    { mask_b = 0b1111; bus_size = 2; }
      if (sig_addr_b[0]) mask_b = mask_b << 1;
      if (sig_addr_b[1]) mask_b = mask_b << 2;

      logic<5>  op_b   = b5(reg_insn_b, 2);

      bus_tla.a_address = sig_addr_b;
      bus_tla.a_data    = rs2_b;
      bus_tla.a_mask    = mask_b;
      bus_tla.a_opcode  = (op_b == RV32I::OP_STORE) ? (bus_size == 2 ? TL::PutFullData : TL::PutPartialData) : TL::Get;
      bus_tla.a_param   = b3(DONTCARE);
      bus_tla.a_size    = bus_size;
      bus_tla.a_source  = b1(DONTCARE);
      bus_tla.a_valid   = (op_b == RV32I::OP_LOAD) || (op_b == RV32I::OP_STORE);
      bus_tla.a_ready   = 1;
    }

    //----------
    // Memory + code/data/reg read/write overrides for cross-thread stuff

    {
      // We write code memory in phase C because it's busy reading the next
      // instruction in phase B.

      // Hmm we can't actually read from code because we also have to read our next instruction
      // and we can't do it earlier or later (we can read it during C, but then it's not back
      // in time to write to the regfile).

      logic<4>       temp_mask_c = 0;
      logic<3> f3_c = b3(reg_insn_c, 12);
      if (f3_c == 0) temp_mask_c = 0b0001;
      if (f3_c == 1) temp_mask_c = 0b0011;
      if (f3_c == 2) temp_mask_c = 0b1111;
      if (reg_addr_c[0]) temp_mask_c = temp_mask_c << 1;
      if (reg_addr_c[1]) temp_mask_c = temp_mask_c << 2;

      logic<4>  bus_tag_c = b4(reg_addr_c, 28);
      logic<1> code_cs_c = bus_tag_c == 0x0 && b24(next_hpc_a) == 0;

      code_tla.a_address  = code_cs_c ? b24(reg_addr_c) : b24(next_hpc_a);
      code_tla.a_data = result_c;
      code_tla.a_mask = temp_mask_c;
      logic<5> op_c = b5(reg_insn_c, 2);
      logic<1> sig_code_wren  = (op_c == RV32I::OP_STORE) && code_cs_c;
      code_tla.a_opcode  = sig_code_wren ? TL::PutFullData : TL::Get;
      code_tla.a_param   = b3(DONTCARE);
      code_tla.a_size    = 2;
      code_tla.a_source  = b1(DONTCARE);
      code_tla.a_valid   = 1;
      code_tla.a_ready   = 1;
    }

    //----------
    // Regfile write

    {
      logic<4>  bus_tag_c = b4(reg_addr_c, 28);
      logic<1>  regfile_cs_c = bus_tag_c == 0xE;
      logic<32> data_out_c = regfile_cs_c ? reg_rdata1 : bus_tld.d_data;
      logic<32> unpacked_c = data_out_c;
      logic<3>  f3_c = b3(reg_insn_c, 12);

      if (result_c[0]) unpacked_c = unpacked_c >> 8;
      if (result_c[1]) unpacked_c = unpacked_c >> 16;
      switch (f3_c) {
        case 0:  unpacked_c = sign_extend<32>( b8(unpacked_c)); break;
        case 1:  unpacked_c = sign_extend<32>(b16(unpacked_c)); break;
        case 4:  unpacked_c = zero_extend<32>( b8(unpacked_c)); break;
        case 5:  unpacked_c = zero_extend<32>(b16(unpacked_c)); break;
      }

      // If we're using jalr to jump between threads, we use the hart from HPC _A_
      // as the target for the write so that the link register will be written
      // in the _destination_ regfile.

      // FIXME why didn't this ternary work?
      //reg_if.waddr = cat(b3(op_c == RV32I::OP_JALR ? reg_hpc_a : reg_hpc_c, 24), rd_c);

      logic<5> op_c = b5(reg_insn_c, 2);
      logic<5> rd_c = b5(reg_insn_c, 7);
      if (op_c == RV32I::OP_JALR) {
        reg_if.waddr = cat(b3(reg_hpc_a, 24), rd_c);
      }
      else {
        reg_if.waddr = cat(b3(reg_hpc_c, 24), rd_c);
      }

      reg_if.wdata = op_c == RV32I::OP_LOAD ? unpacked_c : result_c;
      reg_if.wren = b24(reg_hpc_c) && op_c != RV32I::OP_STORE && op_c != RV32I::OP_BRANCH;

      if (rd_c == 0) reg_if.wren = 0;

      logic<5>  rs1a_b = b5(reg_insn_b, 15);
      logic<32> rs1_b  = rs1a_b ? reg_rdata1 : b32(0);
      logic<32> imm_b = decode_imm(reg_insn_b);
      logic<32> sig_addr_b  = b32(rs1_b + imm_b);
      logic<1> regfile_cs_b = b4(sig_addr_b, 28) == 0xE;
      logic<5>  op_b   = b5(reg_insn_b, 2);
      if ((op_b == RV32I::OP_LOAD) && regfile_cs_b && (b24(reg_hpc_a) == 0)) {
        reg_if.raddr1 = b10(sig_addr_b >> 2);
      }

      // Handle stores through the bus to the regfile.
      if (op_c == RV32I::OP_STORE && regfile_cs_c) {
        reg_if.waddr = b10(reg_addr_c >> 2);
        reg_if.wdata = result_c;
        reg_if.wren = 1;
      }
    }

    //----------
    // Writeback to core regs

    {
      logic<5>  rs1a_b = b5(reg_insn_b, 15);
      logic<32> rs1_b  = rs1a_b ? reg_rdata1 : b32(0);
      logic<32> imm_b = decode_imm(reg_insn_b);
      logic<32> sig_addr_b  = b32(rs1_b + imm_b);
      logic<32> sig_insn_a  = b24(reg_hpc_a) ? code_tld.d_data : b32(0);
      tick(reset_in,
          next_hpc_a, sig_insn_a,
          sig_addr_b, alu_result,
          result_c);
    }
  }


  //----------------------------------------

  tilelink_a bus_tla;
  tilelink_a code_tla;

  regfile_if reg_if;

public:

  //----------------------------------------
  // Internal signals and registers
  // metron_internal

  /* metron_internal */ logic<32> reg_hpc_a;

  /* metron_internal */ logic<32> reg_hpc_b;
  /* metron_internal */ logic<32> reg_insn_b;

  /* metron_internal */ logic<32> reg_hpc_c;
  /* metron_internal */ logic<32> reg_insn_c;
  /* metron_internal */ logic<32> reg_addr_c;
  /* metron_internal */ logic<32> reg_result_c;

  /* metron_internal */ logic<32> reg_hpc_d;
  /* metron_internal */ logic<32> reg_insn_d;
  /* metron_internal */ logic<32> reg_result_d;

  /* metron_internal */ logic<32> reg_ticks;

private:

  //----------------------------------------

  void tick(logic<1> reset_in, logic<32> next_hpc_a, logic<32> sig_insn_a, logic<32> sig_addr_b, logic<32> alu_result, logic<32> sig_result_c) {
    if (reset_in) {
      reg_hpc_a     = 0x00400000;

      reg_hpc_b     = 0;
      reg_insn_b    = 0;

      reg_hpc_c     = 0;
      reg_insn_c    = 0;
      reg_addr_c    = 0;
      reg_result_c  = 0;

      reg_hpc_d     = 0;
      reg_insn_d    = 0;
      reg_result_d  = 0;

      reg_ticks     = 0;
    }
    else {
      //logic<5>  _rs1a_b = b5(reg_insn_b, 15);
      //logic<32> _rs1_b  = _rs1a_b ? reg_rdata1 : b32(0);
      //logic<32> _imm_b = decode_imm(reg_insn_b);
      //logic<32> _sig_addr_b  = b32(_rs1_b + _imm_b);
      //logic<32> _sig_insn_a  = b24(reg_hpc_a) ? code_tld.d_data : b32(0);

      reg_hpc_d     = reg_hpc_c;
      reg_insn_d    = reg_insn_c;
      reg_result_d  = sig_result_c;

      reg_hpc_c     = reg_hpc_b;
      reg_insn_c    = reg_insn_b;
      reg_addr_c    = sig_addr_b;
      reg_result_c  = alu_result;

      reg_hpc_b     = reg_hpc_a;
      reg_insn_b    = sig_insn_a;

      reg_hpc_a     = next_hpc_a;

      reg_ticks     = reg_ticks + 1;
    }
  }

  //----------------------------------------

  logic<32> decode_imm(logic<32> insn) const {
    logic<5>  op    = b5(insn, 2);
    logic<32> imm_i = sign_extend<32>(b12(insn, 20));
    logic<32> imm_s = cat(dup<21>(insn[31]), b6(insn, 25), b5(insn, 7));
    logic<32> imm_u = b20(insn, 12) << 12;
    logic<32> imm_b = cat(dup<20>(insn[31]), insn[7], b6(insn, 25), b4(insn, 8), b1(0));
    logic<32> imm_j = cat(dup<12>(insn[31]), b8(insn, 12), insn[20], b10(insn, 21), b1(0));

    logic<32> result;
    switch(op) {
      case RV32I::OP_LOAD:   result = imm_i; break;
      case RV32I::OP_OPIMM:  result = imm_i; break;
      case RV32I::OP_AUIPC:  result = imm_u; break;
      case RV32I::OP_STORE:  result = imm_s; break;
      case RV32I::OP_OP:     result = imm_i; break;
      case RV32I::OP_LUI:    result = imm_u; break;
      case RV32I::OP_BRANCH: result = imm_b; break;
      case RV32I::OP_JALR:   result = imm_i; break;
      case RV32I::OP_JAL:    result = imm_j; break;
      default:               result = 0;     break;
    }
    return result;
  }

  //----------------------------------------

  logic<32> execute_alu(logic<32> insn, logic<32> reg_a, logic<32> reg_b) const {
    logic<5>  op  = b5(insn, 2);
    logic<3>  f3  = b3(insn, 12);
    logic<7>  f7  = b7(insn, 25);
    logic<32> imm = decode_imm(insn);

    logic<32> alu_a = reg_a;
    logic<32> alu_b = op == RV32I::OP_OPIMM ? imm : reg_b;
    if (op == RV32I::OP_OP && f3 == 0 && f7 == 32) alu_b = -alu_b;

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
      default: result = 0; break;
    }
    return result;
  }

  //----------------------------------------

  logic<32> execute_system(logic<32> insn, logic<32> reg_a, logic<32> reg_b) const {
    logic<3>  f3  = b3(insn, 12);
    logic<12> csr = b12(insn, 20);

    // FIXME need a good error if case is missing an expression
    logic<32> result = 0;
    switch(f3) {
      case RV32I::F3_CSRRW: {
        result = reg_a;
        break;
      }
      case RV32I::F3_CSRRS: {
        if (csr == 0xF14) result = b8(reg_hpc_b, 24);
        break;
      }
      case RV32I::F3_CSRRC:  result = 0; break;
      case RV32I::F3_CSRRWI: result = 0; break;
      case RV32I::F3_CSRRSI: result = 0; break;
      case RV32I::F3_CSRRCI: result = 0; break;
    }
    return result;
  }
};

/* verilator lint_on UNUSEDSIGNAL */
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_PINWHEEL_CORE_H
