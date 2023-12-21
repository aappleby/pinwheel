#ifndef PINWHEEL_UART_UART_RX
#define PINWHEEL_UART_UART_RX

#include "metron/metron_tools.h"

//==============================================================================
// verilator lint_off unusedsignal
// verilator lint_off unusedparam

class uart_rx {
public:

  uart_rx(int clock_rate, int baud_rate) {
    bit_delay_    = (clock_rate / baud_rate) - 1;
    bit_delay_max = (clock_rate / baud_rate) - 1;
    data_buf_ = 0;
    checksum_ = 0;
  }

  // Our output is valid once we've received 8 bits.
  logic<1> get_valid() const {
    return bit_count_ == 8;
  }

  // The most recent data byte received.
  logic<8> get_data_buf() const {
    return data_buf_;
  }
  // True if we have a byte waiting.
  logic<1> get_data_flag() const {
    return data_flag_;
  }

  // The checksum of all bytes received so far.
  logic<32> get_checksum() const {
    return checksum_;
  }

  void tick(
    logic<1> reset,  // Top-level reset signal
    logic<1> serial, // Serial input from the transmitter
    logic<1> byte_consumed
  )
  {
    if (reset) {
      bit_delay_ = bit_delay_max;
      bit_count_ = bit_count_max;
      shift_ = 0;
      checksum_ = 0;
    }
    else {

      if (byte_consumed) data_flag_ = 0;

      // If we're waiting for the next bit to arrive, keep waiting until our
      // bit delay counter runs out.
      if (bit_delay_ < bit_delay_max) {
        bit_delay_ = bit_delay_ + 1;
      }

      // We're done waiting for a bit. If we have bits left to receive, shift
      // them into the top of the output register.
      else if (bit_count_ < bit_count_max) {
        logic<8> new_shift = (serial << 7) | (shift_ >> 1);

        // If that was the last data bit, add the finished byte to our checksum.
        if (bit_count_ == 7) {
          data_buf_ = new_shift;
          data_flag_ = 1;
          checksum_ = checksum_ + new_shift;
        }

        // Move to the next bit and reset our delay counter.
        bit_delay_ = 0;
        bit_count_ = bit_count_ + 1;
        shift_ = new_shift;
      }

      // We're not waiting for a bit and we finished receiving the previous
      // byte. Wait for the serial line to go low, which signals the start of
      // the next byte.
      else if (serial == 0) {
        bit_delay_ = 0;
        bit_count_ = 0;
      }
    }
  }

  //----------------------------------------

 private:

  // We wait for cycles_per_bit cycles
  logic<16> bit_delay_;
  logic<16> bit_delay_max;

  // Our serial data format is 8n1, which is short for "one start bit, 8 data
  // bits, no parity bit, one stop bit". If bit_count_ == 1, we're only waiting
  // on the stop bit.
  static const int bit_count_max = 9;
  static const int bit_count_width = clog2(bit_count_max);
  logic<bit_count_width> bit_count_;

  // The shift register that receives bits
  logic<8> shift_;

  // The most recently received byte
  logic<8> data_buf_;
  logic<1> data_flag_;

  // The checksum of all bytes received so far.
  logic<32> checksum_;
};

// verilator lint_on unusedsignal
// verilator lint_on unusedparam
//==============================================================================

#endif // PINWHEEL_UART_UART_RX
