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
    const char* message_hex  = nullptr,
    int clock_rate = 12000000,
    int baud_rate = 115200,
    int repeat_msg = 1
  )
  : hello(message_hex, repeat_msg),
    tx(clock_rate, baud_rate),
    rx(clock_rate, baud_rate)
  {
  }

  // The actual bit of data currently on the transmitter's output
  logic<1> get_serial() const {
    return tx.get_serial();
  }

  // Returns true if the receiver has a byte in its buffer
  logic<1> get_valid() const {
    return rx.get_valid();
  }

  // The next byte of data from the receiver
  logic<8> get_data_out() const {
    return rx.get_data_out();
  }

  // True if the client has sent its message and the transmitter has finished
  // transmitting it.
  logic<1> get_done() const {
    return hello.get_done() && tx.get_idle();
  }

  // Checksum of all the bytes received
  logic<32> get_checksum() const {
    return rx.get_checksum();
  }

  void tock(logic<1> reset) {
    // Grab signals from our submodules before we tick them.
    logic<8> data    = hello.get_data();
    logic<1> request = hello.get_request();

    logic<1> serial = tx.get_serial();
    logic<1> clear_to_send = tx.get_clear_to_send();
    logic<1> idle = tx.get_idle();

    // Tick all submodules.
    hello.tick(reset, clear_to_send, idle);
    tx.tick(reset, data, request);
    rx.tick(reset, serial);
  }

  //----------------------------------------
private:
  // Our UART client that transmits our "hello world" test message
  uart_hello hello;
  // The UART transmitter
  uart_tx tx;
  // The UART receiver
  uart_rx rx;
};

//==============================================================================

#endif // UART_TOP_H
