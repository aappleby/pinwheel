// This was an idea for a SPI-ish-to-bus-transaction interface for bringing up FPGA components.

#ifndef PINWHEEL_PROBE
#define PINWHEEL_PROBE

#include "metron/metron_tools.h"

//==============================================================================

class Probe {
public:

  logic<32> bus_data_out;
  logic<32> bus_rx_data;
  logic<32> bus_addr;
  logic<4>  bus_width;
  logic<1>  bus_re;
  logic<1>  bus_we;

  logic<8> rx_data;
  logic<1> rx_valid;
  logic<1> rx_ready;

  logic<8> tx_data;
  logic<1> tx_valid;
  logic<8> tx_ready;

  void tick(logic<1> reset) {

    if (reset) {
      @bus_data_out = 0;
      @bus_addr = 0;
      @bus_width = 0;
      @bus_re = 0;
      @bus_we = 0;
      @count_ = 0;
      @state_ = IDLE;
    }
    else {
      logic<4> hex = 0;
      if (rx_data >= '0') hex = b4(rx_data - '0');
      if (rx_data >= 'A') hex = b4(rx_data - 'A' + 10);

      switch(state_) {

        case IDLE:
          if (rx_valid) {
            if (rx_data == 'a') { @count_ = 0; @rx_ready = 1; @state_ = ADDR; }
            if (rx_data == 'd') { @count_ = 0; @rx_ready = 1; @state_ = DATA; }
            if (rx_data == 'r') { @count_ = 0; @rx_ready = 1; @state_ = READ1; }
            if (rx_data == 'w') { @count_ = 0; @rx_ready = 1; @state_ = WRITE1; }
          }
          break;

        case ADDR:
          if (rx_valid) {
            @bus_addr = (bus_addr << 4) | hex;
            @count_ = count_ + 1;
            if (@count_ == 8) {
              @state_ = IDLE;
              @rx_ready = 1;
            }
          }
          break;

        case DATA:
          if (rx_valid) {
            @bus_data = (bus_data << 4) | hex;
            @count_ = count_ + 1;
            if (@count_ == 8) {
              @state_ = IDLE;
              @rx_ready = 1;
            }
          }
          break;

        case READ1:
          if (rx_valid) {
            @bus_width = hex;
            @bus_read = 1;
            @state_ = READ2;
          }
          break;

        case READ2:
          @bus_read = 0;
          @count_ = bus_width;
          @state_ = READ3;
          break;

        case READ3:
          if (bus_read) {
            @temp_ = bus_rx_data;

          }

          logic<8> out = b4(temp_);
          out += (out >= 10) 55 : 48;
          @tx_valid = 1;
          @tx_data  = out;

          if (tx_valid && tx_ready) {
            @temp_ = temp_ >> 4;
            @count_ = count_ - 1;
            if (@count_ == 0) {
              @state_ = IDLE;
              @tx_valid = 0;
              @rx_ready = 1;
            }
          }


      }
    }
  }

private:

  logic<8>  count_;
  logic<8>  state_;
  logic<32> temp_;

  enum {
    IDLE,
    ADDR,
    DATA,
    READ1,
    READ2,
    READ3,
    WRITE1,
    WRITE2,
    WRITE3,
  };

};


//==============================================================================

#endif
