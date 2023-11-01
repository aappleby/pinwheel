#ifndef UART_TOP_H
#define UART_TOP_H

#include "metron/metron_tools.h"
#include "uart_hello.h"
#include "uart_rx.h"
#include "uart_tx.h"

//==============================================================================

template <int cycles_per_bit = 3, int repeat_msg = 0>
class uart_top {
public:

  uart_top(const char* message_hex = "message.hex") : uart0_hello(message_hex) {}

  // The actual bit of data currently on the transmitter's output
  logic<1> get_serial() const {
    return uart0_tx.get_serial();
  }

  // Returns true if the receiver has a byte in its buffer
  logic<1> get_valid() const {
    return uart0_rx.get_valid();
  }

  // The next byte of data from the receiver
  logic<8> get_data_out() const {
    return uart0_rx.get_data_out();
  }

  // True if the client has sent its message and the transmitter has finished
  // transmitting it.
  logic<1> get_done() const {
    return uart0_hello.get_done() && uart0_tx.get_idle();
  }

  // Checksum of all the bytes received
  logic<32> get_checksum() const {
    return uart0_rx.get_checksum();
  }

  void tock(logic<1> reset) {
    // Grab signals from our submodules before we tick them.
    logic<8> data    = uart0_hello.get_data();
    logic<1> request = uart0_hello.get_request();

    logic<1> serial        = uart0_tx.get_serial();
    logic<1> clear_to_send = uart0_tx.get_clear_to_send();
    logic<1> idle          = uart0_tx.get_idle();

    // Tick all submodules.
    uart0_hello.tick(reset, clear_to_send, idle);
    uart0_tx.tick(reset, data, request);
    uart0_rx.tick(reset, serial);
  }

  //----------------------------------------
private:
  // Our UART client that transmits our "hello world" test message
  uart_hello<repeat_msg>  uart0_hello;
  // The UART transmitter
  uart_tx<cycles_per_bit> uart0_tx;
  // The UART receiver
  uart_rx<cycles_per_bit> uart0_rx;
};

//==============================================================================

#endif // UART_TOP_H
