#pragma once
#include "metron_tools.h"

#include "block_ram.h"
// metron_noconvert
#include "console.h"
#include "constants.h"
#include "pinwheel_core.h"
#include "tilelink.h"

// Address Map
// 0x0xxxxxxx - Code
// 0x8xxxxxxx - Data
// 0xExxxxxxx - Regfiles
// 0xFxxxxxxx - Debug registers

// verilator lint_off unusedsignal

//------------------------------------------------------------------------------

class pinwheel {
public:

  pinwheel(const char* text_file = nullptr, const char* data_file = nullptr) : code_ram(text_file), data_ram(data_file) {
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

  void tock(logic<1> reset_in, logic<1> _serial_valid, logic<8> _serial_data) {

    logic<32> code_to_core = code_ram.rdata();
    logic<32> bus_to_core  = data_ram.rdata();

    if (debug_reg_cs) bus_to_core = debug_reg;
    if (serial_cs)    bus_to_core = serial_reg;

    //----------

    core.tock(reset_in, code_to_core, bus_to_core, regs.get_rs1(), regs.get_rs2());
    logic<4> bus_tag_b = b4(core.sig_bus_addr, 28);

    //----------

    regs.tick(core.sig_rf_raddr1, core.sig_rf_raddr2, core.sig_rf_waddr, core.sig_rf_wdata, core.sig_rf_wren);

    {
      logic<1> debug_cs_b = bus_tag_b == 0xF;
      debug_reg_next = debug_reg;
      debug_reg_cs_next = debug_cs_b;
      if (core.sig_bus_wren && debug_cs_b) debug_reg_next = core.sig_bus_wdata;
    }

    {
      serial_cs_next = 0;
      serial_valid_next = 0;
      serial_reg_next = 0;
      serial_out_next = 0;
      serial_out_valid_next = 0;
    }

    /*
    {
      logic<1> serial_cs_b = bus_tag_b == 0xC;

      if (core.sig_bus_wren && serial_cs_b) {
        serial_out_next = core.sig_bus_wdata;
        serial_out_valid_next = 1;
      } else {
        serial_out_next = 0;
        serial_out_valid_next = 0;
      }

      if (core.sig_bus_rden && serial_cs_b) {
        serial_cs_next = serial_cs_b;
      }
      else {
        serial_cs_next = 0;
      }
    }
    */

    code_ram.tick(b12(core.sig_code_addr), 1,                core.sig_code_wdata, core.sig_code_wmask, core.sig_code_wren && bus_tag_b == 0x0);
    data_ram.tick(b12(core.sig_bus_addr),  bus_tag_b == 0x8, core.sig_bus_wdata,  core.sig_bus_wmask,  core.sig_bus_wren  && bus_tag_b == 0x8);

    // metron_noconvert
    console1.tick(reset_in, bus_tag_b == 0x4 && core.sig_bus_wren, core.sig_bus_wdata);
    // metron_noconvert
    console2.tick(reset_in, bus_tag_b == 0x5 && core.sig_bus_wren, core.sig_bus_wdata);
    // metron_noconvert
    console3.tick(reset_in, bus_tag_b == 0x6 && core.sig_bus_wren, core.sig_bus_wdata);
    // metron_noconvert
    console4.tick(reset_in, bus_tag_b == 0x7 && core.sig_bus_wren, core.sig_bus_wdata);
  }

  //----------------------------------------
  // FIXME trace modules individually

  void tick(logic<1> reset_in, logic<1> _serial_valid, logic<8> _serial_data) {
    if (reset_in) {
      debug_reg = 0;
      debug_reg_cs = 0;
      serial_cs = 0;
      serial_out = 0;
      serial_out_valid = 0;
      serial_reg = 0;
    }
    else {
      debug_reg = debug_reg_next;
      debug_reg_cs = debug_reg_cs_next;

      serial_cs        = serial_cs_next;
      serial_valid     = serial_valid_next;
      serial_reg       = serial_reg_next;
      serial_out       = serial_out_next;
      serial_out_valid = serial_out_valid_next;
    }
  }

  //----------------------------------------
  // metron_internal

  pinwheel_core core;
  regfile       regs;

  logic<32> debug_reg_next;
  logic<32> debug_reg;
  logic<1>  debug_reg_cs_next;
  logic<1>  debug_reg_cs;

  block_ram code_ram;
  block_ram data_ram; // FIXME having this named data and a field inside block_ram named data breaks context resolve

  logic<32> gpio_dir;
  logic<32> gpio_in;
  logic<32> gpio_out;

  logic<1>  serial_cs_next;
  logic<1>  serial_valid_next;
  logic<32> serial_reg_next;
  logic<32> serial_out_next;
  logic<1>  serial_out_valid_next;

  logic<1>  serial_cs;
  logic<1>  serial_valid;
  logic<32> serial_reg;
  logic<32> serial_out;
  logic<1>  serial_out_valid;

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
