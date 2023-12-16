#ifndef PINWHEEL_UART_UART_TX_H
#define PINWHEEL_UART_UART_TX_H

#include "metron/metron_tools.h"

//==============================================================================
// verilator lint_off unusedsignal
// verilator lint_off unusedparam

class uart_tx {
public:
  uart_tx(int clock_rate, int baud_rate) {
    bit_delay_     = (clock_rate / baud_rate) - 1;
    bit_delay_max_ = (clock_rate / baud_rate) - 1;
    bit_count_ = bit_count_max;
    shift_ = 0;
  }

  // The actual bit of data we're sending to the serial port.
  logic<1> get_serial() const {
    return shift_ & 1;
  }

  // True if the transmitter is ready to accept another byte.
  logic<1> get_clear_to_send() const {
    return ((bit_count_ == bit_count_done) && (bit_delay_ == bit_delay_max_)) ||
           (bit_count_ > bit_count_done);
  }

  // True if the transmitter has sent the message plus the extra stop bits.
  logic<1> get_idle() const {
    return (bit_count_ == bit_count_max) && (bit_delay_ == bit_delay_max_);
  }

  void tick(logic<1> reset, logic<8> send_data, logic<1> send_request) {
    if (reset) {
      bit_delay_ = bit_delay_max_;
      bit_count_ = bit_count_max;
      shift_ = 0x1FF;
    }

    else {
      // If we've just sent a bit, wait for the delay to expire before sending
      // another.
      if (bit_delay_ < bit_delay_max_) {
        bit_delay_ = bit_delay_ + 1;
      }

      // The bit delay is done. If we have more bits to send, shift our output
      // buffer over and append a stop bit.
      else if (bit_count_ < bit_count_done) {
        bit_delay_ = 0;
        bit_count_ = bit_count_ + 1;
        shift_ = (shift_ >> 1) | 0x100;
      }

      // If we don't have any more bits to send, check for a new send request.
      else if (send_request) {
        bit_delay_ = 0;
        bit_count_ = 0;
        // We shift the new byte left by one so that the low 0 bit in the output
        // buffer serves as the start bit for the next byte.
        shift_ = send_data << 1;
      }

      // If there was no send request, keep sending extra stop bits until we've
      // sent enough.
      else if (bit_count_ < bit_count_max) {
        bit_delay_ = 0;
        bit_count_ = bit_count_ + 1;
      }
    }
  }

  //----------------------------------------

private:

  // We wait {cycles_per_bit} cycles between sending bits.
  logic<16> bit_delay_;
  logic<16> bit_delay_max_;

  // We send 1 start bit, 8 data bits, and 1 stop bit per byte = 10 bits per
  // byte total. We also send 7 additional stop bits between messages to
  // guarantee that the receiver can resynchronize with our start bit.

  static const int bit_count_done  = 10;
  static const int extra_stop_bits = 7;
  static const int bit_count_width = clog2(10 + extra_stop_bits);
  static const int bit_count_max   = bit_count_done + extra_stop_bits;
  logic<bit_count_width> bit_count_;

  // Our output shift register is 9 (not 8) bits wide so that the low bit can
  // serve as our start bit.
  logic<9> shift_;
};

// verilator lint_on unusedsignal
// verilator lint_on unusedparam
//==============================================================================

#endif // PINWHEEL_UART_UART_TX_H
