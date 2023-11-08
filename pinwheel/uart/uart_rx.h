#ifndef PINWHEEL_UART_UART_RX
#define PINWHEEL_UART_UART_RX

#include "metron/metron_tools.h"
#include "pinwheel/tools/tilelink.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off unusedparam

// 0xB0000000 = data ready
// 0xB0000004 = data
// 0xB0000008 = checksum

template <uint32_t addr_mask = 0xF000F000, uint32_t addr_tag = 0xB0000000, int cycles_per_bit = 4>
class uart_rx {
public:

  // Our output is valid once we've received 8 bits.
  logic<1> get_valid() const {
    return bit_count == 8;
  }

  // The most recent data byte received.
  logic<8> get_data_out() const {
    return data_out;
  }

  // The checksum of all bytes received so far.
  logic<32> get_checksum() const {
    return checksum;
  }

  // FIXME should this be happening in tick? Probably?

  void tock(const logic<1> reset, const logic<1> serial, const tilelink_a tla)
  {
    tld.d_opcode = b3(DONTCARE);
    tld.d_param  = 0; // required by spec
    tld.d_size   = 2;
    tld.d_source = 0;
    tld.d_sink   = 0;
    tld.d_data   = b32(DONTCARE);
    tld.d_error  = 0;
    tld.d_valid  = 0;
    tld.d_ready  = 1;

    logic<1> byte_consumed = 0;

    if (tla.a_valid && (tla.a_address & addr_mask) == addr_tag && tla.a_opcode == TL::Get) {
      /* metron_noconvert */
      tld.d_opcode = TL::AccessAckData;
      tld.d_error  = 0;

      switch(tla.a_address & 0xF) {
        case 0x0000:
          tld.d_data  = b32(data_flag);
          tld.d_valid = 1;
          break;
        case 0x0004:
          //tld.d_data  = b32(data_out);
          tld.d_data = b32(data_buf);
          tld.d_valid = 1;
          byte_consumed = 1;
          break;
        case 0x0008:
          tld.d_data  = checksum;
          tld.d_valid = 1;
          break;
      }
    }

    tick(reset, serial, byte_consumed);
  }

  tilelink_d tld;

 private:

  void tick(
    logic<1> reset,  // Top-level reset signal
    logic<1> serial, // Serial input from the transmitter
    logic<1> byte_consumed
  )
  {
    if (reset) {
      bit_delay = bit_delay_max;
      bit_count = bit_count_max;
      data_out = 0;
      checksum = 0;
    }
    else {

      // If we're waiting for the next bit to arrive, keep waiting until our
      // bit delay counter runs out.
      if (bit_delay < bit_delay_max) {
        bit_delay = bit_delay + 1;
      }

      // We're done waiting for a bit. If we have bits left to receive, shift
      // them into the top of the output register.
      else if (bit_count < bit_count_max) {
        logic<8> new_output = (serial << 7) | (data_out >> 1);

        // If that was the last data bit, add the finished byte to our checksum.
        if (bit_count == 7) {
          data_buf = new_output;
          data_flag = 1;
          checksum = checksum + new_output;
        }

        // Move to the next bit and reset our delay counter.
        bit_delay = 0;
        bit_count = bit_count + 1;
        data_out = new_output;
      }

      // We're not waiting for a bit and we finished receiving the previous
      // byte. Wait for the serial line to go low, which signals the start of
      // the next byte.
      else if (serial == 0) {
        bit_delay = 0;
        bit_count = 0;
      }
    }

    if (byte_consumed) data_flag = 0;
  }


  // We wait for cycles_per_bit cycles
  static const int bit_delay_width = clog2(cycles_per_bit);
  static const int bit_delay_max = cycles_per_bit - 1;
  logic<bit_delay_width> bit_delay;

  // Our serial data format is 8n1, which is short for "one start bit, 8 data
  // bits, no parity bit, one stop bit". If bit_count == 1, we're only waiting
  // on the stop bit.
  static const int bit_count_max = 9;
  static const int bit_count_width = clog2(bit_count_max);
  logic<bit_count_width> bit_count;

  // The received byte
  logic<8> data_out;

  logic<8> data_buf;
  logic<1> data_flag;

  // The checksum of all bytes received so far.
  logic<32> checksum;
};

// verilator lint_on unusedsignal
// verilator lint_on unusedparam
//------------------------------------------------------------------------------

#endif // PINWHEEL_UART_UART_RX
