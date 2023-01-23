#pragma once
#include "metron_tools.h"

#include "block_ram.h"
#include "pinwheel_core.h"
#include "regfile.h"
#include "serial.h"
#include "test_reg.h"
#include "tilelink.h"

// metron_noconvert
#include "console.h"

// Address Map
// 0x0xxxxxxx - Code
// 0x8xxxxxxx - Data
// 0xExxxxxxx - Regfiles
// 0xFxxxxxxx - Debug registers

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off undriven

class pinwheel {
public:

  // FIXME debug_reg2(0x1234) is here because icarus doesn't like it if we don't assign module params
  pinwheel(const char* text_file = nullptr, const char* data_file = nullptr)
  : code_ram(text_file),
    data_ram(data_file) {
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
  logic<32> get_debug() const { return debug_reg2.get(); }

  //----------------------------------------
  // FIXME const local variable should not become parameter


  void tock(logic<1> reset_in, logic<1> _serial_valid, logic<8> _serial_data) {

    logic<32> bus_to_core  = data_ram.bus_tld.d_data;
    if (serial_cs)    bus_to_core = serial_reg;

    if (debug_reg2.bus_tld.d_valid == 1) bus_to_core = serial_reg;

    //----------

    core.tock(reset_in, code_ram.bus_tld.d_data, bus_to_core, regs.get_rs1(), regs.get_rs2());
    logic<4> bus_tag_b = b4(core.sig_bus_addr, 28);

    tilelink_a bus_tla;
    bus_tla.a_opcode  = core.sig_bus_wren ? TL::PutPartialData : TL::Get;
    bus_tla.a_param   = b3(DONTCARE);
    bus_tla.a_size    = 0; // fixme
    bus_tla.a_source  = b1(DONTCARE);
    bus_tla.a_address = core.sig_bus_addr;
    bus_tla.a_mask    = core.sig_bus_wmask;
    bus_tla.a_data    = core.sig_bus_wdata;
    bus_tla.a_valid   = 1;
    bus_tla.a_ready   = 1;

    //----------


    debug_reg2.tick(bus_tla);

    {
      serial_cs_next = 0;
      serial_valid_next = 0;
      serial_reg_next = 0;
      serial_out_next = 0;
      serial_out_valid_next = 0;
    }

    {
      tilelink_a code_tla;
      code_tla.a_opcode  = core.sig_code_wren ? TL::PutPartialData : TL::Get;
      code_tla.a_param   = b3(DONTCARE);
      code_tla.a_size    = 2;
      code_tla.a_source  = b1(DONTCARE);
      code_tla.a_address = core.sig_code_addr;
      code_tla.a_mask    = core.sig_code_wmask;
      code_tla.a_data    = core.sig_code_wdata;
      code_tla.a_valid   = 1;
      code_tla.a_ready   = 1;

      code_ram.tick(code_tla);
    }

    data_ram.tick(bus_tla);

    core.tick(reset_in);
    regs.tick(core.sig_rf_raddr1, core.sig_rf_raddr2, core.sig_rf_waddr, core.sig_rf_wdata, core.sig_rf_wren);

    // metron_noconvert
    console1.tick(reset_in, bus_tla);
    // metron_noconvert
    console2.tick(reset_in, bus_tla);
    // metron_noconvert
    console3.tick(reset_in, bus_tla);
    // metron_noconvert
    console4.tick(reset_in, bus_tla);
  }

  //----------------------------------------
  // FIXME trace modules individually

  void tick(logic<1> reset_in, logic<1> _serial_valid, logic<8> _serial_data) {
    if (reset_in) {
      serial_cs = 0;
      serial_out = 0;
      serial_out_valid = 0;
      serial_reg = 0;
    }
    else {
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

  test_reg <0xF0000000, 0xF0000000> debug_reg2;

  block_ram<0xF0000000, 0x00000000> code_ram;
  block_ram<0xF0000000, 0x80000000> data_ram; // FIXME having this named data and a field inside block_ram named data breaks context resolve

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
  Console<0xF0000000, 0x40000000> console1;
  // metron_noconvert
  Console<0xF0000000, 0x50000000> console2;
  // metron_noconvert
  Console<0xF0000000, 0x60000000> console3;
  // metron_noconvert
  Console<0xF0000000, 0x70000000> console4;
};

// verilator lint_on unusedsignal
// verilator lint_off undriven
//------------------------------------------------------------------------------
