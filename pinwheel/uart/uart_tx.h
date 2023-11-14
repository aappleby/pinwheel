#ifndef PINWHEEL_UART_UART_TX_H
#define PINWHEEL_UART_UART_TX_H

#include "metron/metron_tools.h"
#include "pinwheel/tools/tilelink.h"

// 0xB0000000 = send ready
// 0xB0000004 = data

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off unusedparam

template <uint32_t addr_mask = 0xF000F000, uint32_t addr_tag = 0xB0000000>
class uart_tx {
public:
  uart_tx(logic<16> cycles_per_bit) {
    bit_delay     = cycles_per_bit - 1;
    bit_delay_max = cycles_per_bit - 1;
    bit_count = bit_count_max;
    output_buffer = 0;

    tld.d_opcode = b3(DONTCARE);
    tld.d_param  = b2(DONTCARE);
    tld.d_size   = b3(DONTCARE);
    tld.d_source = b1(DONTCARE);
    tld.d_sink   = b3(DONTCARE);
    tld.d_data   = b32(DONTCARE);
    tld.d_error  = 0;
    tld.d_valid  = 0;
    tld.d_ready  = 1;
  }

  // The actual bit of data we're sending to the serial port.
  logic<1> get_serial() const {
    return output_buffer & 1;
  }

  // True if the transmitter is ready to accept another byte.
  logic<1> get_clear_to_send() const {
    return ((bit_count == bit_count_done) && (bit_delay == bit_delay_max)) ||
           (bit_count > bit_count_done);
  }

  // True if the transmitter has sent the message plus the extra stop bits.
  logic<1> get_idle() const {
    return (bit_count == bit_count_max) && (bit_delay == bit_delay_max);
  }

  void tock(const logic<1> reset, const logic<8> send_data, const logic<1> send_request, const tilelink_a tla) {
    tld.d_opcode = b3(DONTCARE);
    tld.d_param  = 0; // required by spec
    tld.d_size   = 2;
    tld.d_source = 0;
    tld.d_sink   = 0;
    tld.d_data   = b32(DONTCARE);
    tld.d_error  = 0;
    tld.d_valid  = 0;
    tld.d_ready  = 1;

    logic<8> next_data = send_data;
    logic<1> next_req  = send_request;

    if (tla.a_valid && (tla.a_address & addr_mask) == addr_tag) {
      // Read
      if (tla.a_opcode == TL::Get) {
        switch(tla.a_address & 0xF) {
          case 0x0000: {
            logic<1> ctx = ((bit_count == bit_count_done) && (bit_delay == bit_delay_max)) || (bit_count > bit_count_done);
            tld.d_opcode = TL::AccessAckData;
            tld.d_data   = b32(ctx);
            tld.d_valid  = 1;
            break;
          }
          default: {
            tld.d_error = 1;
            break;
          }
        }
      }

      // Write
      else if (tla.a_opcode == TL::PutFullData) {
        switch(tla.a_address & 0xF) {
          case 0x0004: {
            next_data = b8(tla.a_data);
            next_req  = 1;
            tld.d_opcode = TL::AccessAck;
            tld.d_valid  = 1;
            break;
          }
          default: {
            tld.d_error = 1;
            break;
          }
        }
      }
    }

    tick(reset, next_data, next_req);
  }

  tilelink_d tld;

private:

  void tick(logic<1> reset, logic<8> send_data, logic<1> send_request) {
    if (reset) {
      bit_delay = bit_delay_max;
      bit_count = bit_count_max;
      output_buffer = 0x1FF;
    }

    else {
      // If we've just sent a bit, wait for the delay to expire before sending
      // another.
      if (bit_delay < bit_delay_max) {
        bit_delay = bit_delay + 1;
      }

      // The bit delay is done. If we have more bits to send, shift our output
      // buffer over and append a stop bit.
      else if (bit_count < bit_count_done) {
        bit_delay = 0;
        bit_count = bit_count + 1;
        output_buffer = (output_buffer >> 1) | 0x100;
      }

      // If we don't have any more bits to send, check for a new send request.
      else if (send_request) {
        bit_delay = 0;
        bit_count = 0;
        // We shift the new byte left by one so that the low 0 bit in the output
        // buffer serves as the start bit for the next byte.
        output_buffer = send_data << 1;
      }

      // If there was no send request, keep sending extra stop bits until we've
      // sent enough.
      else if (bit_count < bit_count_max) {
        bit_delay = 0;
        bit_count = bit_count + 1;
      }
    }
  }

  // We wait {cycles_per_bit} cycles between sending bits.
  logic<16> bit_delay;
  logic<16> bit_delay_max;

  // We send 1 start bit, 8 data bits, and 1 stop bit per byte = 10 bits per
  // byte total. We also send 7 additional stop bits between messages to
  // guarantee that the receiver can resynchronize with our start bit.

  static const int bit_count_done  = 10;
  static const int extra_stop_bits = 7;
  static const int bit_count_width = clog2(10 + extra_stop_bits);
  static const int bit_count_max   = bit_count_done + extra_stop_bits;
  logic<bit_count_width> bit_count;

  // Our output buffer is 9 (not 8) bits wide so that the low bit can serve as
  // our start bit.
  logic<9> output_buffer;
};

// verilator lint_on unusedsignal
// verilator lint_on unusedparam
//------------------------------------------------------------------------------

#endif // PINWHEEL_UART_UART_TX_H
