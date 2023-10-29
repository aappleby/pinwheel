#ifndef PINWHEEL_RTL_PINWHEEL_H
#define PINWHEEL_RTL_PINWHEEL_H

#include "metron/metron_tools.h"

#include "pinwheel/core/pinwheel_core.h"
#include "pinwheel/soc/block_ram.h"
#include "pinwheel/soc/regfile.h"
#include "pinwheel/soc/test_reg.h"
#include "pinwheel/tools/tilelink.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off undriven

class pinwheel {
public:

  pinwheel(
    const char* code_hexfile = "pinwheel/tools/blank.code.vh",
    const char* data_hexfile = "pinwheel/tools/blank.data.vh"
  ) : code_ram(code_hexfile), data_ram(data_hexfile) {
  }

  /*metron_noconvert*/ pinwheel* clone();
  /*metron_noconvert*/ size_t size_bytes();
  /*metron_noconvert*/ bool load_elf(const char* firmware_filename);
  /*metron_noconvert*/ uint32_t* get_code();
  /*metron_noconvert*/ uint32_t* get_data();
  /*metron_noconvert*/ logic<32> get_debug() const;

  //----------------------------------------
  // FIXME const local variable should not become parameter

  // FIXME yosys can't handle structs as local variables

  void tock(logic<1> reset_in, logic<1> _serial_valid, logic<8> _serial_data) {
    bus_tld.d_opcode = b3(DONTCARE);
    bus_tld.d_param  = b2(DONTCARE);
    bus_tld.d_size   = b3(DONTCARE);
    bus_tld.d_source = b1(DONTCARE);
    bus_tld.d_sink   = b3(DONTCARE);
    bus_tld.d_data   = b32(DONTCARE);
    bus_tld.d_error  = b1(DONTCARE);
    bus_tld.d_valid  = b1(DONTCARE);
    bus_tld.d_ready  = b1(DONTCARE);

    if (data_ram.bus_tld.d_valid == 1)  bus_tld = data_ram.bus_tld;
    if (debug_reg.bus_tld.d_valid == 1) bus_tld = debug_reg.bus_tld;

    //----------

    core.tock(reset_in, code_ram.bus_tld, bus_tld, regs.get_rs1(), regs.get_rs2());

    debug_reg.tick(core.bus_tla);
    code_ram.tick(core.code_tla);
    data_ram.tick(core.bus_tla);
    core.tick(reset_in);

    regs.tick(core.core_to_reg);
  }

  //----------------------------------------
  // FIXME trace modules individually

  void tick(logic<1> reset_in, logic<1> _serial_valid, logic<8> _serial_data) {
  }

  //----------------------------------------

  /* metron_internal */ tilelink_d    bus_tld;
  /* metron_internal */ pinwheel_core core;
  /* metron_internal */ regfile       regs;
  /* metron_internal */ block_ram<0xF0000000, 0x00000000> code_ram;
  /* metron_internal */ block_ram<0xF0000000, 0x80000000> data_ram; // FIXME having this named data and a field inside block_ram named data breaks context resolve
  /* metron_internal */ test_reg <0xF0000000, 0xF0000000> debug_reg;
};

// verilator lint_on unusedsignal
// verilator lint_off undriven
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_PINWHEEL_H
