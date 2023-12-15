#ifndef UART_HELLO_H
#define UART_HELLO_H

#include "metron/metron_tools.h"

//==============================================================================

class uart_hello {
public:
  uart_hello(const char* message_filename = nullptr, int repeat_msg = 1)
  : repeat_msg(repeat_msg) {
    readmemh(message_filename, memory_, 0, 511);
    state_ = 0;
    cursor_ = 0;
  }

  // The byte of data we want transmitted is always the one at the cursor.
  logic<8> get_data() const {
    return memory_[cursor_];
  }

  // True if we want to transmit a byte
  logic<1> get_request() const {
    return state_ == SEND;
  }

  // True if we've transmitted the whole message.
  logic<1> get_done() const {
    return state_ == DONE;
  }

  void tick(
    logic<1> reset,          // Top-level reset signal.
    logic<1> clear_to_send,  // True if the transmitter can accept an input byte
    logic<1> idle)           // True if the transmitter is idle
  {
    // In reset we're always in WAIT state with the message cursor set to
    // the start of the message buffer.
    if (reset) {
      state_ = WAIT;
      cursor_ = 0;
    }
    else {
      // If we're waiting for the transmitter to be free and it's told us that
      // it's idle, go to SEND state.
      if (state_ == WAIT && idle) {
        state_ = SEND;
      }

      // If we're currently sending a message and the transmitter is ready to
      // accept another byte,
      else if (state_ == SEND && clear_to_send) {
        // either go to DONE state if we're about to send the last character of
        // the message
        if (cursor_ == message_len - 1) {
          state_ = DONE;
        }

        // or just advance the message cursor.
        else {
          cursor_ = cursor_ + 1;
        }
      }

      // If we've finished transmitting, reset the message cursor and either go
      // back to WAIT state if we want to re-transmit or just stay in DONE
      // otherwise.
      else if (state_ == DONE) {
        cursor_ = 0;
        if (repeat_msg) state_ = WAIT;
      }
    }
  }

private:
  static const int message_len = 512;
  static const int cursor_bits = clog2(message_len);

  static const int WAIT = 0; // Waiting for the transmitter to be free
  static const int SEND = 1; // Sending the message buffer
  static const int DONE = 2; // Message buffer sent
  logic<2> state_;            // One of the above states
  logic<1> repeat_msg;

  logic<8> memory_[512];      // The buffer preloaded with our message
  logic<cursor_bits> cursor_; // Index into the message buffer of the _next_ character to transmit
};

//==============================================================================

#endif // UART_HELLO_H
