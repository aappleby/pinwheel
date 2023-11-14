#ifndef UART_TOP_H
#define UART_TOP_H

#include "metron/metron_tools.h"
#include "uart_hello.h"
#include "uart_rx.h"
#include "uart_tx.h"

//==============================================================================

class uart_top {
public:

  uart_top(
    const char* message_hex  = "message.hex",
    int clock_rate = 12000000,
    int baud_rate = 115200,
    int repeat_msg = 1
  )
  :
  uart0_hello(message_hex, repeat_msg),
  uart0_tx(clock_rate, baud_rate),
  uart0_rx(clock_rate, baud_rate)
  {
  }

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
    uart0_rx.tock(reset, uart0_tx.get_serial());

    logic<1> clear_to_send = uart0_tx.get_clear_to_send();
    logic<1> idle = uart0_tx.get_idle();

    logic<8> data = uart0_hello.get_data();
    logic<1> request = uart0_hello.get_request();

    uart0_tx.tock(reset, data, request);
    uart0_hello.tock(reset, clear_to_send, idle);
  }

  //----------------------------------------
private:
  // Our UART client that transmits our "hello world" test message
  uart_hello uart0_hello;
  // The UART transmitter
  uart_tx<0xF000F000, 0xB0000000> uart0_tx;
  // The UART receiver
  uart_rx<0xF000F000, 0xB0000000> uart0_rx;
};

//==============================================================================

#endif // UART_TOP_H
