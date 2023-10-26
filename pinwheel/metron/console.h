#ifndef PINWHEEL_RTL_CONSOLE_H
#define PINWHEEL_RTL_CONSOLE_H

#include "metron/metron_tools.h"
#include "pinwheel/metron/tilelink.h"

// Pseudo-hardware, not actually synthesizable

//------------------------------------------------------------------------------
// verilator lint_off unusedparam

class Console {
public:
  Console(uint32_t addr_mask, uint32_t addr_tag) : addr_mask(addr_mask), addr_tag(addr_tag) {
    memset(buf, 0, sizeof(buf));
  }

  void tick(logic<1> reset, tilelink_a tla) {
    if (reset) {
      memset(buf, 0, sizeof(buf));
      x = 0;
      y = 0;
    }
    else {

      if (((tla.a_address & addr_mask) == addr_tag) && (tla.a_opcode == TL::PutPartialData || tla.a_opcode == TL::PutFullData)) {
        buf[y * width + x] = 0;
        auto c = char(tla.a_data);

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
          for (int i = 0; i < width*(height-1); i++) {
            buf[i] = buf[i + width];
          }
          memset(buf + (width*(height-1)), 0, width);
          y = height-1;
        }
        buf[y * width + x] = 30;
      }
    }
  }

  static const int width =64;
  static const int height=16;

  const uint32_t addr_mask;
  const uint32_t addr_tag;
  char buf[width*height];
  int  x = 0;
  int  y = 0;
};

// verilator lint_on unusedparam
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_CONSOLE_H
