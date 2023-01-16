#pragma once
#include "metron_tools.h"

#include "block_ram.h"
// metron_noconvert
#include "console.h"
#include "constants.h"
#include "pinwheel_core.h"

//------------------------------------------------------------------------------

class pinwheel {
public:

  pinwheel(const char* text_file = nullptr, const char* data_file = nullptr) {
    readmemh(text_file, code_ram.data);
    readmemh(data_file, data_ram.data);
  }

  // metron_noconvert
  pinwheel* clone() {
    pinwheel* p = new pinwheel();
    memcpy(p, this, sizeof(*this));
    return p;
  }

  // metron_noconvert
  size_t size_bytes() { return sizeof(*this); }
  // metron_noconvert
  bool load_elf(const char* firmware_filename);
  // metron_noconvert
  uint32_t* get_code() { return code_ram.get_data(); }
  // metron_noconvert
  uint32_t* get_data() { return data_ram.get_data(); }
  // metron_noconvert
  logic<32> get_debug() const { return debug_reg; }

  //----------------------------------------
  // FIXME const local variable should not become parameter

  void tock(logic<1> reset_in) {

    logic<32> code_to_core = code_ram.rdata();
    logic<32> bus_to_core  = data_ram.rdata();

    if (debug_reg_cs) bus_to_core = debug_reg;

    core.tock(reset_in, code_to_core, bus_to_core);
  }

  //----------------------------------------
  // FIXME trace modules individually

  void tick(logic<1> reset_in) {

    logic<4> bus_tag_b = b4(core.bus_addr, 28);

    if (reset_in) {
      debug_reg = 0;
      debug_reg_cs = 0;
    }
    else {
      logic<1> debug_cs_b = bus_tag_b == 0xF;
      if (core.bus_wren && debug_cs_b) debug_reg = core.bus_wdata;
      debug_reg_cs = debug_cs_b;
    }

    logic<32> code_to_core = code_ram.rdata();
    logic<32> bus_to_core  = data_ram.rdata();

    code_ram.tick(b12(core.code_addr), 1,                core.code_wdata, core.code_wmask, core.code_wren && bus_tag_b == 0x0);
    data_ram.tick(b12(core.bus_addr),  bus_tag_b == 0x8, core.bus_wdata,  core.bus_wmask,  core.bus_wren  && bus_tag_b == 0x8);

    // metron_noconvert
    console1.tick(reset_in, bus_tag_b == 0x4 && core.bus_wren, core.bus_wdata);
    // metron_noconvert
    console2.tick(reset_in, bus_tag_b == 0x5 && core.bus_wren, core.bus_wdata);
    // metron_noconvert
    console3.tick(reset_in, bus_tag_b == 0x6 && core.bus_wren, core.bus_wdata);
    // metron_noconvert
    console4.tick(reset_in, bus_tag_b == 0x7 && core.bus_wren, core.bus_wdata);

    core.tick(reset_in, code_to_core, bus_to_core);
  }

  //----------------------------------------

  // metron_internal
  pinwheel_core core;

  logic<32> debug_reg;
  logic<1>  debug_reg_cs;

  block_ram code_ram;
  block_ram data_ram; // FIXME having this named data and a field inside block_ram named data breaks context resolve

  logic<32> gpio_dir;
  logic<32> gpio_in;
  logic<32> gpio_out;

  // metron_noconvert
  Console console1;
  // metron_noconvert
  Console console2;
  // metron_noconvert
  Console console3;
  // metron_noconvert
  Console console4;
};

// verilator lint_on unusedsignal

//------------------------------------------------------------------------------
