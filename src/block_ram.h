#pragma once
#include "metron_tools.h"

//------------------------------------------------------------------------------

class BlockRam {
public:

  void tock(logic<32> addr_, logic<32> wdata_, logic<4> wmask_, logic<1> wren_) {
    addr  = addr_;
    wdata = wdata_;
    wmask = wmask_;
    wren  = wren_;
  }

  logic<32> rdata() const {
    return data[b10(addr, 2)];
  }

  void tick() {
    if (wren) {
      logic<32> old_data = data[b10(addr, 2)];
      logic<32> new_data = wdata;
      if (addr[0]) new_data = new_data << 8;
      if (addr[1]) new_data = new_data << 16;

      data[b10(addr, 2)] = ((wmask[0] ? new_data : old_data) & 0x000000FF) |
                           ((wmask[1] ? new_data : old_data) & 0x0000FF00) |
                           ((wmask[2] ? new_data : old_data) & 0x00FF0000) |
                           ((wmask[3] ? new_data : old_data) & 0xFF000000);
    }
  }

  // noconvert
  uint32_t* get_data() { return data; }
  // noconvert
  const uint32_t* get_data() const { return data; }

private:

  logic<32> addr;
  logic<32> wdata;
  logic<4>  wmask;
  logic<1>  wren;

  uint32_t  data[16384];
};

//------------------------------------------------------------------------------
