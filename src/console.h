#pragma once
#include "metron_tools.h"

//------------------------------------------------------------------------------

class Console {
public:

  void tock(logic<1> wrcs_, logic<32> reg_b_) {
    wrcs = wrcs_;
    reg_b = reg_b_;
  }

  void tick(logic<1> reset) {
    if (reset) {
      memset(buf, 0, sizeof(buf));
      x = 0;
      y = 0;
    }
    else if (wrcs) {
      buf[y * width + x] = 0;
      auto c = char(reg_b);

      if (c == 0) c = '?';

      if (c == '\n') {
        x = 0;
        y++;
      }
      else if (c == '\r') {
        x = 0;
      }
      else {
        buf[y * width + x] = c;
        x++;
      }

      if (x == width) {
        x = 0;
        y++;
      }
      if (y == height) {
        memcpy(buf, buf + width, width*(height-1));
        memset(buf + (width*(height-1)), 0, width);
        y = height-1;
      }
      buf[y * width + x] = 30;
    }
  }

  static const int width =64;
  static const int height=16;

  char buf[width*height];
  int  x = 0;
  int  y = 0;
  logic<1>  wrcs;
  logic<32> reg_b;
};

//------------------------------------------------------------------------------
