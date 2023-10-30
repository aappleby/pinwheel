#ifndef PINWHEEL_RTL_PINWHEEL_H
#define PINWHEEL_RTL_PINWHEEL_H

#include "metron/metron_tools.h"

#include "pinwheel/core/pinwheel_core.h"
#include "pinwheel/soc/block_ram.h"
#include "pinwheel/soc/regfile.h"
#include "pinwheel/soc/test_reg.h"
#include "pinwheel/tools/tilelink.h"

#include "pinwheel/uart/uart_hello.h"
#include "pinwheel/uart/uart_rx.h"
#include "pinwheel/uart/uart_tx.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off undriven

class pinwheel_soc {
public:

  pinwheel_soc(
    const char* code_hexfile = "pinwheel/tools/blank.code.vh",
    const char* data_hexfile = "pinwheel/tools/blank.data.vh",
    const char* message_hex  = "pinwheel/uart/message.hex")
  : code_ram(code_hexfile), data_ram(data_hexfile), hello(message_hex) {
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

    //----------

    core.tock(reset_in, code_ram.bus_tld, bus_tld, regs.get_rs1(), regs.get_rs2());

    debug_reg.tick(core.bus_tla);
    code_ram.tick(core.code_tla);
    data_ram.tick(core.bus_tla);
    core.tick(reset_in);

    regs.tick(core.core_to_reg);

    {
      // Grab signals from our submodules before we tick them.
      logic<8> data = hello.get_data();
      logic<1> request = hello.get_request();

      logic<1> serial = tx.get_serial();
      logic<1> clear_to_send = tx.get_clear_to_send();
      logic<1> idle = tx.get_idle();

      hello.tick(reset_in, clear_to_send, idle);
      tx.tick(reset_in, data, request);
      rx.tick(reset_in, serial);
    }
  }

  //----------------------------------------
  // FIXME trace modules individually

  void tick(logic<1> reset_in) {
  }

  //----------------------------------------

  /* metron_internal */ tilelink_d    bus_tld;
  /* metron_internal */ pinwheel_core core;
  /* metron_internal */ regfile       regs;
  /* metron_internal */ block_ram<0xF0000000, 0x00000000> code_ram;
  /* metron_internal */ block_ram<0xF0000000, 0x80000000> data_ram; // FIXME having this named data and a field inside block_ram named data breaks context resolve
  /* metron_internal */ test_reg <0xF0000000, 0xF0000000> debug_reg;


  // The actual bit of data currently on the transmitter's output
  /* metron_noconvert*/ logic<1> get_serial() const {
    return tx.get_serial();
  }

  // Returns true if the receiver has a byte in its buffer
  /* metron_noconvert*/ logic<1> get_valid() const {
    return rx.get_valid();
  }

  // The next byte of data from the receiver
  /* metron_noconvert*/ logic<8> get_data_out() const {
    return rx.get_data_out();
  }

  // True if the client has sent its message and the transmitter has finished
  // transmitting it.
  /* metron_noconvert*/ logic<1> get_done() const {
    return hello.get_done() && tx.get_idle();
  }

  // Checksum of all the bytes received
  /* metron_noconvert*/ logic<32> get_checksum() const {
    return rx.get_checksum();
  }

  // Our UART client that transmits our "hello world" test message
  /* metron_internal */ uart_hello<false /*repeat_msg*/>  hello;
  // The UART transmitter
  /* metron_internal */ uart_tx<3 /*cycles_per_bit*/> tx;
  // The UART receiver
  /* metron_internal */ uart_rx<3 /*cycles_per_bit*/> rx;

};

// verilator lint_on unusedsignal
// verilator lint_off undriven
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_PINWHEEL_H
