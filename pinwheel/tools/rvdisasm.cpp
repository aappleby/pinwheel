#include "metrolib/core/Check.h"
#include "metrolib/core/Dumper.h"
#include "metron/metron_tools.h"
#include "pinwheel/metron/riscv_constants.h"
#include "pinwheel/tools/rvdisasm.h"

void print_rv(Dumper& d, uint32_t op_u32) {
  logic<32> op = op_u32;

  if (op == 0) {
    d(R"(<???>)");
    return;
  }

  if (b2(op) != 0b11) {
    d(R"(<???>)");
    return;
  }

  uint32_t opcode = b5(op, 2);

  auto rd  = b5(op, 7);
  auto rs1 = b5(op, 15);
  auto rs2 = b5(op, 20);

  auto f3 = b3(op, 12);
  auto f7 = b7(op, 25);

  auto imm_i = b12(op, 20).as_signed();
  auto imm_s = cat(dup<21>(op[31]), b6(op, 25), b5(op, 7)).as_signed();
  auto imm_b = cat(dup<20>(op[31]), op[7], b6(op, 25), b4(op, 8), b1(0)).as_signed();
  auto imm_u = b20(op, 12) << 12;
  auto imm_j = cat(dup<12>(op[31]), b8(op, 12), op[20], b10(op, 21), b1(0)).as_signed();

  auto imm_i_abs = imm_i < 0 ? -imm_i : imm_i;
  auto imm_s_abs = imm_s < 0 ? -imm_s : imm_s;
  auto imm_b_abs = imm_b < 0 ? -imm_b : imm_b;
  auto imm_j_abs = imm_j < 0 ? -imm_j : imm_j;

  switch (opcode) {
    // Load
    case RV32I::OP_LOAD: {
      switch(f3) {
      case 0: d("LB    "); break;
      case 1: d("LH    "); break;
      case 2: d("LW    "); break;
      case 3: d("L??   "); break;
      case 4: d("LBU   "); break;
      case 5: d("LHU   "); break;
      case 6: d("L??   "); break;
      case 7: d("L??   "); break;
      }
      d("r%02d, [r%02d%c%d]", rd, rs1, imm_i < 0 ? '-' : '+', imm_i_abs); break;
      break;
    }

    // Store
    case RV32I::OP_STORE: {
      switch(f3) {
      case 0: d("SB    "); break;
      case 1: d("SH    "); break;
      case 2: d("SW    "); break;
      case 3: d("S??   "); break;
      case 4: d("S??   "); break;
      case 5: d("S??   "); break;
      case 6: d("S??   "); break;
      case 7: d("S??   "); break;
      }
      d("r%02d, [r%02d%c%d]", rs2, rs1, imm_s < 0 ? '-' : '+', imm_s_abs); break;
      break;
    }

    // ALU
    case RV32I::OP_OP: {
      switch(f3) {
      case 0: b1(op,30) ? d("SUB   ") : d("ADD   "); break;
      case 1: d("SLL   "); break;
      case 2: d("SLT   "); break;
      case 3: d("SLTU  "); break;
      case 4: d("XOR   "); break;
      case 5: b1(op,30) ? d("SRA   ") : d("SRL   "); break;
      case 6: d("OR    "); break;
      case 7: d("AND   "); break;
      }
      d("r%02d, r%02d, r%02d", rd, rs1, rs2); break;
      break;
    }

    // ALUI
    case RV32I::OP_OPIMM: {
      switch(f3) {
      case 0: d("ADDI  "); break;
      case 1: d("SLLI  "); break;
      case 2: d("SLTI  "); break;
      case 3: d("SLTUI "); break;
      case 4: d("XORI  "); break;
      case 5: b1(op, 30) ? d("SRAI  ") : d("SRLI  "); break;
      case 6: d("ORI   "); break;
      case 7: d("ANDI  "); break;
      }
      d("r%02d, r%02d, %d", rd, rs1, imm_i); break;
      break;
    }

    // Branch
    case RV32I::OP_BRANCH: {
      switch(f3) {
      case 0: d("BEQ   "); break;
      case 1: d("BNE   "); break;
      case 2: d("???   "); break;
      case 3: d("???   "); break;
      case 4: d("BLT   "); break;
      case 5: d("BGE   "); break;
      case 6: d("BLTU  "); break;
      case 7: d("BGEU  "); break;
      }
      d("r%02d, r%02d, %c%d", rs1, rs2, imm_b < 0 ? '-' : '+', imm_b_abs);
      break;
    }

    // Add upper immediate to PC
    case RV32I::OP_AUIPC: {
      d("AUIPC r%02d, 0x%08x", rd, imm_u);
      break;
    }

    // Load upper immediate
    case RV32I::OP_LUI: {
      d("LUI   r%02d, %d", b5(op, 7), b20(op, 12));
      break;
    }

    // Jump relative to register and link
    case RV32I::OP_JALR: {
      d("JALR  r%02d, [r%02d%c%d]", rd,  rs1, imm_i < 0 ? '-' : '+', imm_i_abs);
      break;
    }

    // Jump absolute and link
    case RV32I::OP_JAL: {
      d("JAL   r%02d, [%c%d]", rd, imm_j < 0 ? '-' : '+', imm_j_abs);
      break;
    }

    case RV32I::OP_SYSTEM: {
      switch(f3) {
      case 0: d("???   "); break;
      case 1: d("CSRRW "); break;
      case 2: d("CSRRS "); break;
      case 3: d("CSRRC "); break;
      case 4: d("???   "); break;
      case 5: d("CSRWI "); break;
      case 6: d("CSRSI "); break;
      case 7: d("CSRCI "); break;
      }
      d("r%02d, 0x%03x, r%02d", rd, (int)b12(op, 20), rs1);
      break;
    }

    case RV32I::OP_CUSTOM0: {
      d("CUSTOM! rd=%d rs1 = %d rs2 = %d", rd, rs1, rs2);
      break;
    }

    //case 0b00011: d("MISC_MEM "); break;
    //case 0b00110: d("OPIMM32  "); break;
    //case 0b01110: d("OP32     "); break;
    //case 0b00111: d("48b      "); break;
    //case 0b01001: d("STOREFP  "); break;
    //case 0b01010: d("CUSTOM1  "); break;
    //case 0b01011: d("AMO      "); break;
    //case 0b01111: d("64B      "); break;
    //case 0b10000: d("MADD     "); break;
    //case 0b10001: d("MSUB     "); break;
    //case 0b10010: d("NMSUB    "); break;
    //case 0b10011: d("NMMADD   "); break;
    //case 0b10100: d("OPFP     "); break;
    //case 0b10101: d("RESERVED1"); break;
    //case 0b10110: d("CUSTOM2  "); break;
    //case 0b10111: d("48b      "); break;
    //case 0b11010: d("RESERVED2"); break;
    //case 0b11100: d("SYSTEM   "); break;
    //case 0b11101: d("RESERVED3"); break;
    //case 0b11110: d("CUSTOM3  "); break;
    //case 0b11111: d("80b      "); break;
    //case 0b00001: d("LOADFP   "); break;
    //case 0b00010: d("CUSTOM0  "); break;
    default:
      d(R"(<???>)");
      //printf("what op is this??? %d 0x%08x\n", opcode, op.as_unsigned());
      //debugbreak();
      break;
  }
}
