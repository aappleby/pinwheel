#pragma once
#include "metron_tools.h"

//------------------------------------------------------------------------------

class block_ram {
public:

  void tock(logic<12> addr_, logic<32> wdata_, logic<4> wmask_, logic<1> wren_) {
    addr  = addr_;
    wdata = wdata_;
    wmask = wmask_;
    wren  = wren_;
  }

  logic<32> rdata() const {
    return data_out;
  }

  void tick() {
    if (wren) {
      logic<32> old_data = data[b10(addr, 2)];
      logic<32> new_data = wdata;
      if (addr[0]) new_data = new_data << 8;
      if (addr[1]) new_data = new_data << 16;
      new_data = ((wmask[0] ? new_data : old_data) & 0x000000FF) |
                 ((wmask[1] ? new_data : old_data) & 0x0000FF00) |
                 ((wmask[2] ? new_data : old_data) & 0x00FF0000) |
                 ((wmask[3] ? new_data : old_data) & 0xFF000000);

      data[b10(addr, 2)] = new_data;
      data_out = new_data;
    }
    else {
      data_out = data[b10(addr, 2)];
    }
  }

  // metron_noconvert
  uint32_t* get_data() { return (uint32_t*)data; }
  // metron_noconvert
  const uint32_t* get_data() const { return (uint32_t*)data; }

  // metron_internal
  logic<12> addr;
  logic<32> wdata;
  logic<4>  wmask;
  logic<1>  wren;

  logic<32> data[16384];
  logic<32> data_out;
};

//------------------------------------------------------------------------------
