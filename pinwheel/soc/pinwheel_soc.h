#ifndef PINWHEEL_RTL_PINWHEEL_H
#define PINWHEEL_RTL_PINWHEEL_H

#include "metron/metron_tools.h"

#include "pinwheel/core/pinwheel_core.h"
#include "pinwheel/soc/bus_ram.h"
#include "pinwheel/soc/regfile.h"
#include "pinwheel/soc/test_reg.h"
#include "pinwheel/tools/tilelink.h"

#include "pinwheel/uart/uart_hello.h"
#include "pinwheel/uart/uart_tilelink.h"

//==============================================================================
// verilator lint_off unusedsignal
// verilator lint_off undriven

template<int code_dwords = 1024, int data_dwords = 1024>
class pinwheel_soc {
public:

  pinwheel_soc(
    const char* code_hexfile = "pinwheel/tools/blank.code.vh",
    const char* data_hexfile = "pinwheel/tools/blank.data.vh",
    const char* message_hex  = "pinwheel/uart/message.hex",
    int clock_rate = 12000000,
    int baud_rate = 115200,
    int repeat_msg = 1
  )
  : code_ram(code_hexfile),
    data_ram(data_hexfile),
    uart0_tx(clock_rate, baud_rate, 0xFFFF'0000, 0xB000'0000),
    uart0_rx(clock_rate, baud_rate, 0xFFFF'0000, 0xB001'0000),
    uart0_hello(message_hex, repeat_msg)
  {
  }

  // FIXME why does this hang yosys if exposed?

  /*metron_noconvert*/ logic<32> get_debug() { return debug_reg.get_tld().d_data; }

  logic<1> get_uart() { return uart0_tx.get_serial(); }

  /* metron_internal */ tilelink_d get_data_tld() {
    tilelink_d data_tld     = data_ram.get_tld();
    tilelink_d debug_tld    = debug_reg.get_tld();
    tilelink_d uart0_tx_tld = uart0_tx.get_tld();
    tilelink_d uart0_rx_tld = uart0_rx.get_tld();

    tilelink_d result;

    if      (debug_tld.d_valid    == 1) result = debug_tld;
    else if (uart0_tx_tld.d_valid == 1) result = uart0_tx_tld;
    else if (uart0_rx_tld.d_valid == 1) result = uart0_rx_tld;
    else                                result = data_tld;

    return result;
  }

  //----------------------------------------
  // FIXME const local variable should not become parameter

  void tock(logic<1> reset_in) {
    tilelink_d code_tld = code_ram.get_tld();
    tilelink_d data_tld = get_data_tld();

    core.tock(reset_in, code_tld, data_tld, regs.get_rs1(), regs.get_rs2());

    uart0_rx.tock(reset_in, uart0_tx.get_serial(), core.data_tla);

    debug_reg.tock(core.data_tla);
    code_ram.tock_b(core.code_tla);
    data_ram.tock_b(core.data_tla);
    regs.tick(core.reg_if);

    logic<1> ready = uart0_tx.get_ready();
    logic<1> idle = uart0_tx.get_idle();

    logic<8> data = uart0_hello.get_data();
    logic<1> request = uart0_hello.get_request();

    uart0_tx.tock(reset_in, data, request, core.data_tla);
    uart0_hello.tick(reset_in, ready, idle);
  }

  //----------------------------------------

  /* metron_internal */ pinwheel_core core;
  /* metron_internal */ regfile       regs;

  /* metron_internal */ bus_ram   <0xF000'0000, 0x0000'0000, code_dwords> code_ram;  // Code  at 0x0xxx'xxxx
  /* metron_internal */ bus_ram   <0xF000'0000, 0x8000'0000, data_dwords> data_ram;  // Data  at 0x8xxx'xxxx
  /* metron_internal */ test_reg  <0xF000'0000, 0xF000'0000> debug_reg; // Debug at 0xFxxx'xxxx

  /* metron_internal */ uart_tx_tilelink uart0_tx;
  /* metron_internal */ uart_rx_tilelink uart0_rx;

  /* metron_internal */ uart_hello uart0_hello;
};

// verilator lint_on unusedsignal
// verilator lint_off undriven
//==============================================================================

#endif // PINWHEEL_RTL_PINWHEEL_H
