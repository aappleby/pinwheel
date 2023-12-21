#ifndef PINWHEEL_RTL_CONSOLE_H
#define PINWHEEL_RTL_CONSOLE_H

#include "metron/metron_tools.h"
#include "pinwheel/tools/tilelink.h"

// Pseudo-hardware, not actually synthesizable

//------------------------------------------------------------------------------
// verilator lint_off unusedparam

class Console {
public:

  Console() {
    addr_mask = 0;
    addr_tag = 0;
    x_ = 0;
    y_ = 0;
  }

  void init(uint32_t addr_mask, uint32_t addr_tag) {
    this->addr_mask = addr_mask;
    this->addr_tag = addr_tag;
    memset(buf_, 0, sizeof(buf_));
  }

  void putchar(char c) {
    buf_[y_ * width + x_] = 0;

    if (c == 0) c = '?';

    if (c == '\n') {
      x_ = 0;
      y_++;
    }
    else if (c == '\r') {
      x_ = 0;
    }
    else {
      buf_[y_ * width + x_] = c;
      x_++;
    }

    if (x_ == width) {
      x_ = 0;
      y_++;
    }
    if (y_ == height) {
      for (int i = 0; i < width*(height-1); i++) {
        buf_[i] = buf_[i + width];
      }
      memset(buf_ + (width*(height-1)), 0, width);
      y_ = height-1;
    }
    buf_[y_ * width + x_] = 30;
  }

  void tick(logic<1> reset, tilelink_a tla) {
    if (reset) {
      memset(buf_, 0, sizeof(buf_));
      x_ = 0;
      y_ = 0;
    }
    else {
      if ((tla.a_address & addr_mask) == addr_tag) {
        if (tla.a_opcode == TL::PutPartialData || tla.a_opcode == TL::PutFullData) {
          buf_[y_ * width + x_] = 0;
          putchar(char(tla.a_data));
        }
      }
    }
  }

  static const int width =64;
  static const int height=16;

  uint32_t addr_mask;
  uint32_t addr_tag;
  char buf_[width*height];
  int x_;
  int y_;
};

// verilator lint_on unusedparam
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_CONSOLE_H
