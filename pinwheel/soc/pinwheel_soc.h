#ifndef PINWHEEL_RTL_PINWHEEL_H
#define PINWHEEL_RTL_PINWHEEL_H

#include "metron/metron_tools.h"

#include "pinwheel/core/pinwheel_core.h"
#include "pinwheel/soc/block_ram.h"
#include "pinwheel/soc/regfile.h"
#include "pinwheel/soc/test_reg.h"
#include "pinwheel/tools/tilelink.h"

#include "pinwheel/uart/uart_hello.h"
#include "pinwheel/uart/uart_tx.h"
#include "pinwheel/uart/uart_rx.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off undriven

class pinwheel_soc {
public:

  pinwheel_soc(
    const char* code_hexfile = "pinwheel/tools/blank.code.vh",
    const char* data_hexfile = "pinwheel/tools/blank.data.vh",
    const char* message_hex  = "pinwheel/uart/message.hex",
    int clk_rate = 12000000,
    int uart_bps = 1000000
  )
  : code_ram(code_hexfile),
    data_ram(data_hexfile),
    uart0_tx(clk_rate / uart_bps),
    uart0_rx(clk_rate / uart_bps),
    uart0_hello(message_hex)
  {

    bus_tla.a_opcode  = b3(DONTCARE);
    bus_tla.a_param   = b3(DONTCARE);
    bus_tla.a_size    = b3(DONTCARE);
    bus_tla.a_source  = b1(DONTCARE);
    bus_tla.a_address = b32(DONTCARE);
    bus_tla.a_mask    = b4(DONTCARE);
    bus_tla.a_data    = b32(DONTCARE);
    bus_tla.a_valid   = b1(DONTCARE);
    bus_tla.a_ready   = b1(DONTCARE);

    bus_tld.d_opcode = b3(DONTCARE);
    bus_tld.d_param  = b2(DONTCARE);
    bus_tld.d_size   = b3(DONTCARE);
    bus_tld.d_source = b1(DONTCARE);
    bus_tld.d_sink   = b3(DONTCARE);
    bus_tld.d_data   = b32(DONTCARE);
    bus_tld.d_error  = b1(DONTCARE);
    bus_tld.d_valid  = b1(DONTCARE);
    bus_tld.d_ready  = b1(DONTCARE);
  }

  // FIXME why does this hang yosys if exposed?

  /*metron_noconvert*/ logic<32> get_debug() { return debug_reg.get(); }

  //----------------------------------------
  // FIXME const local variable should not become parameter

  // FIXME yosys can't handle structs as local variables

  void tock(logic<1> reset_in) {
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
    if (uart0_rx.tld.d_valid == 1)      bus_tld = uart0_rx.tld;

    //----------

    bus_tla = core.tock_data_bus(reset_in, regs.get_rs1(), regs.get_rs2());
    core.tock(reset_in, code_ram.bus_tld, bus_tld, regs.get_rs1(), regs.get_rs2());

    uart0_rx.tock(reset_in, uart0_tx.get_serial(), bus_tla);

    debug_reg.tock(bus_tla);
    code_ram.tock(core.code_tla);
    data_ram.tock(bus_tla);
    regs.tock(core.reg_if);

    logic<1> clear_to_send = uart0_tx.get_clear_to_send();
    logic<1> idle = uart0_tx.get_idle();

    logic<8> data = uart0_hello.get_data();
    logic<1> request = uart0_hello.get_request();

    uart0_tx.tock(reset_in, data, request, bus_tla);
    uart0_hello.tock(reset_in, clear_to_send, idle);
  }

  //----------------------------------------

  /* metron_internal */ tilelink_a    bus_tla;
  /* metron_internal */ tilelink_d    bus_tld;
  /* metron_internal */ pinwheel_core core;
  /* metron_internal */ regfile       regs;


  /* metron_internal */ block_ram <0xF000'0000, 0x0000'0000> code_ram;  // Code  at 0x0xxx'xxxx
  /* metron_internal */ block_ram <0xF000'0000, 0x8000'0000> data_ram;  // Data  at 0x8xxx'xxxx
  /* metron_internal */ test_reg  <0xF000'0000, 0xF000'0000> debug_reg; // Debug at 0xFxxx'xxxx
  /* metron_internal */ uart_tx   <0xFFFF'0000, 0xB000'0000> uart0_tx;  // Uart TX  0xB000'xxxx
  /* metron_internal */ uart_rx   <0xFFFF'0000, 0xB001'0000> uart0_rx;  // Uart RX  0xB001'xxxx

  /* metron_internal */ uart_hello<false /*repeat_msg*/>  uart0_hello;
};

// verilator lint_on unusedsignal
// verilator lint_off undriven
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_PINWHEEL_H
