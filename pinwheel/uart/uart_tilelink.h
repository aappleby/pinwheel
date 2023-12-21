#ifndef PINWHEEL_UART_TILELINK_H
#define PINWHEEL_UART_TILELINK_H

#include "metron/metron_tools.h"
#include "pinwheel/tools/tilelink.h"
#include "pinwheel/uart/uart_tx.h"
#include "pinwheel/uart/uart_rx.h"

// TileLink adapters for our trivial UART.

// TX @ 0x00 = clear to send
// TX @ 0x04 = data

// RX @ 0x00 = data ready
// RX @ 0x04 = data
// RX @ 0x08 = checksum

//==============================================================================
// verilator lint_off unusedsignal
// verilator lint_off unusedparam

class uart_tx_tilelink {
public:

  uart_tx_tilelink(
    int clock_rate,
    int baud_rate,
    logic<32> _addr_mask,
    logic<32> _addr_tag
  )
  : tx(clock_rate, baud_rate)
  {
    addr_mask = _addr_mask;
    addr_tag  = _addr_tag;
    tld_.d_opcode = b32(DONTCARE);
    tld_.d_param  = b2(DONTCARE);
    tld_.d_size   = b3(DONTCARE);
    tld_.d_source = b1(DONTCARE);
    tld_.d_sink   = b3(DONTCARE);
    tld_.d_data   = b32(DONTCARE);
    tld_.d_error  = b1(DONTCARE);
    tld_.d_valid  = 0;
    tld_.d_ready  = 1;
  }

  logic<1> get_serial() const        { return tx.get_serial(); }
  logic<1> get_clear_to_send() const { return tx.get_clear_to_send(); }
  logic<1> get_idle() const          { return tx.get_idle(); }

  //----------------------------------------

  void tock(const logic<1> reset, logic<8> hello_data, logic<1> hello_req, const tilelink_a tla) {
    logic<8> next_data = hello_data;
    logic<1> next_req  = hello_req;

    if (tla.a_valid &&
       ((tla.a_address & addr_mask) == addr_tag) &&
       (tla.a_opcode == TL::PutFullData) &&
       (tla.a_address & 0xF) == 0x0004)
    {
      next_data = b8(tla.a_data);
      next_req  = 1;
    }

    tick(reset, tla);
    tx.tick(reset, next_data, next_req);
  }

  //----------------------------------------

  void tick(const logic<1> reset, const tilelink_a tla) {
    tld_.d_opcode = b3(DONTCARE);
    tld_.d_param  = 0; // required by spec
    tld_.d_size   = 2;
    tld_.d_source = 0;
    tld_.d_sink   = 0;
    tld_.d_data   = b32(DONTCARE);
    tld_.d_error  = 0;
    tld_.d_valid  = 0;
    tld_.d_ready  = 1;

    if (tla.a_valid && (tla.a_address & addr_mask) == addr_tag) {
      // Read
      if (tla.a_opcode == TL::Get) {
        switch(tla.a_address & 0xF) {
          case 0x0000: {
            tld_.d_opcode = TL::AccessAckData;
            tld_.d_data   = b32(tx.get_clear_to_send());
            tld_.d_valid  = 1;
            break;
          }
          default: {
            tld_.d_error = 1;
            break;
          }
        }
      }

      // Write
      else if (tla.a_opcode == TL::PutFullData) {
        switch(tla.a_address & 0xF) {
          case 0x0004: {
            tld_.d_opcode = TL::AccessAck;
            tld_.d_valid  = 1;
            break;
          }
          default: {
            tld_.d_error = 1;
            break;
          }
        }
      }
    }
  }

  //----------------------------------------

  tilelink_d get_tld() const { return tld_; }

  //----------------------------------------

private:

  uart_tx tx;
  logic<32> addr_mask;
  logic<32> addr_tag;
  tilelink_d tld_;
};

//------------------------------------------------------------------------------

class uart_rx_tilelink {
public:

  uart_rx_tilelink(
    int clock_rate,
    int baud_rate,
    logic<32> _addr_mask,
    logic<32> _addr_tag
  ) : rx(clock_rate, baud_rate)
  {
    addr_mask = _addr_mask;
    addr_tag  = _addr_tag;
    tld_.d_opcode = b32(DONTCARE);
    tld_.d_param  = b2(DONTCARE);
    tld_.d_size   = b3(DONTCARE);
    tld_.d_source = b1(DONTCARE);
    tld_.d_sink   = b3(DONTCARE);
    tld_.d_data   = b32(DONTCARE);
    tld_.d_error  = b1(DONTCARE);
    tld_.d_valid  = 0;
    tld_.d_ready  = 1;
  }

  logic<1>  get_valid() const     { return rx.get_valid(); }
  logic<8>  get_data_buf() const  { return rx.get_data_buf(); }
  logic<1>  get_data_flag() const { return rx.get_data_flag(); }
  logic<32> get_checksum() const  { return rx.get_checksum(); }

  //----------------------------------------

  void tock(const logic<1> reset, const logic<1> serial, const tilelink_a tla)
  {
    logic<1> byte_consumed = 0;

    if (tla.a_valid &&
       ((tla.a_address & addr_mask) == addr_tag) &&
       (tla.a_opcode == TL::Get) &&
       ((tla.a_address & 0xF) == 0x0004))
    {
      byte_consumed = 1;
    }

    tick (reset, tla);
    rx.tick(reset, serial, byte_consumed);
  }

  //----------------------------------------

  void tick(const logic<1> reset, const tilelink_a tla)
  {
    tld_.d_opcode = b3(DONTCARE);
    tld_.d_param  = 0; // required by spec
    tld_.d_size   = 2;
    tld_.d_source = 0;
    tld_.d_sink   = 0;
    tld_.d_data   = b32(DONTCARE);
    tld_.d_error  = 0;
    tld_.d_valid  = 0;
    tld_.d_ready  = 1;

    if (tla.a_valid && (tla.a_address & addr_mask) == addr_tag && tla.a_opcode == TL::Get) {
      tld_.d_opcode = TL::AccessAckData;
      tld_.d_error  = 0;

      switch(tla.a_address & 0xF) {
        case 0x0000: {
          tld_.d_data  = b32(rx.get_data_flag());
          tld_.d_valid = 1;
          break;
        }
        case 0x0004: {
          tld_.d_data  = b32(rx.get_data_buf());
          tld_.d_valid = 1;
          break;
        }
        case 0x0008: {
          tld_.d_data  = rx.get_checksum();
          tld_.d_valid = 1;
          break;
        }
      }
    }
  }

  //----------------------------------------

  tilelink_d get_tld() const {
    return tld_;
  }

  //----------------------------------------

 private:

  uart_rx rx;
  logic<32> addr_mask;
  logic<32> addr_tag;
  tilelink_d tld_;
};

// verilator lint_on unusedsignal
// verilator lint_on unusedparam
//==============================================================================

#endif // PINWHEEL_UART_TILELINK_H
