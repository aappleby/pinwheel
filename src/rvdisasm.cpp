#include "rvdisasm.h"
#include "metron_tools.h"

void print_rv(uint32_t op) {
  uint32_t opcode = b5(op, 2);
  switch (opcode) {
    case 0b00000: printf("LOAD     "); break;
    case 0b00001: printf("LOADFP   "); break;
    case 0b00010: printf("CUSTOM0  "); break;
    case 0b00011: printf("MISC_MEM "); break;
    case 0b00100: printf("OPIMM    "); break;
    case 0b00101: printf("AUIPC    "); break;
    case 0b00110: printf("OPIMM32  "); break;
    case 0b00111: printf("48b      "); break;
    case 0b01000: printf("STORE    "); break;
    case 0b01001: printf("STOREFP  "); break;
    case 0b01010: printf("CUSTOM1  "); break;
    case 0b01011: printf("AMO      "); break;
    case 0b01100: printf("OP       "); break;
    case 0b01101: printf("LUI      "); break;
    case 0b01110: printf("OP32     "); break;
    case 0b01111: printf("64B      "); break;
    case 0b10000: printf("MADD     "); break;
    case 0b10001: printf("MSUB     "); break;
    case 0b10010: printf("NMSUB    "); break;
    case 0b10011: printf("NMMADD   "); break;
    case 0b10100: printf("OPFP     "); break;
    case 0b10101: printf("RESERVED1"); break;
    case 0b10110: printf("CUSTOM2  "); break;
    case 0b10111: printf("48b      "); break;
    case 0b11000: printf("BRANCH   "); break;
    case 0b11001: printf("JALR     "); break;
    case 0b11010: printf("RESERVED2"); break;
    case 0b11011: printf("JAL      "); break;
    case 0b11100: printf("SYSTEM   "); break;
    case 0b11101: printf("RESERVED3"); break;
    case 0b11110: printf("CUSTOM3  "); break;
    case 0b11111: printf("80b      "); break;
  }
}
