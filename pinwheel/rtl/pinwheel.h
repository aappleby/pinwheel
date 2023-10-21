#ifndef PINWHEEL_RTL_PINWHEEL_H
#define PINWHEEL_RTL_PINWHEEL_H

#include "metron/metron_tools.h"

#include "pinwheel/rtl/block_ram.h"
#include "pinwheel/rtl/pinwheel_core.h"
#include "pinwheel/rtl/regfile.h"
#include "pinwheel/rtl/serial.h"
#include "pinwheel/rtl/test_reg.h"
#include "pinwheel/rtl/tilelink.h"

// metron_XXX_noconvert
//#include "console.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off undriven

class pinwheel {
public:

  pinwheel(const char* text_file = nullptr, const char* data_file = nullptr)
  : code_ram(text_file),
    data_ram(data_file) {
  }

  // metron_XXX_noconvert
  /*
  pinwheel* clone() {
    pinwheel* p = new pinwheel();
    memcpy(p, this, sizeof(*this));
    return p;
  }
  */

  /*
  // metron_XXX_noconvert
  size_t size_bytes() { return sizeof(*this); }
  // metron_XXX_noconvert
  bool load_elf(const char* firmware_filename);
  // metron_XXX_noconvert
  uint32_t* get_code() { return code_ram.get_data(); }
  // metron_XXX_noconvert
  uint32_t* get_data() { return data_ram.get_data(); }
  // metron_XXX_noconvert
  logic<32> get_debug() const { return debug_reg.get(); }
  */

  //----------------------------------------
  // FIXME const local variable should not become parameter

  void tock(logic<1> reset_in, logic<1> _serial_valid, logic<8> _serial_data) {

    tilelink_d bus_tld;
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

    /*
    // metron_XXX_noconvert
    console1.tick(reset_in, core.bus_tla);
    // metron_XXX_noconvert
    console2.tick(reset_in, core.bus_tla);
    // metron_XXX_noconvert
    console3.tick(reset_in, core.bus_tla);
    // metron_XXX_noconvert
    console4.tick(reset_in, core.bus_tla);
    */
  }

  //----------------------------------------
  // FIXME trace modules individually

  void tick(logic<1> reset_in, logic<1> _serial_valid, logic<8> _serial_data) {
  }

  //----------------------------------------
  // metron_internal

  pinwheel_core core;
  regfile       regs;

  block_ram<0xF0000000, 0x00000000> code_ram;
  block_ram<0xF0000000, 0x80000000> data_ram; // FIXME having this named data and a field inside block_ram named data breaks context resolve
  test_reg <0xF0000000, 0xF0000000> debug_reg;

  /*
  // metron_XXX_noconvert
  Console  <0xF0000000, 0x40000000> console1;
  // metron_XXX_noconvert
  Console  <0xF0000000, 0x50000000> console2;
  // metron_XXX_noconvert
  Console  <0xF0000000, 0x60000000> console3;
  // metron_XXX_noconvert
  Console  <0xF0000000, 0x70000000> console4;
  */
};

// verilator lint_on unusedsignal
// verilator lint_off undriven
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_PINWHEEL_H
