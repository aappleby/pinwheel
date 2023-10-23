#ifndef PINWHEEL_RTL_CONSOLE_H
#define PINWHEEL_RTL_CONSOLE_H

#include "metron/metron_tools.h"
#include "pinwheel/metron/tilelink.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedparam

template <uint32_t addr_mask = 0xF0000000, uint32_t addr_tag = 0x00000000>
class console {
public:

  void tick(logic<1> reset, tilelink_a tla) {
    /*
    if (reset) {
      memset(buf, 0, sizeof(buf));
      x = 0;pinwheel
      y = 0;
    }
    else {

      if (((tla.a_address & addr_mask) == addr_tag) && (tla.a_opcode == TL::PutPartialData)) {
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
          memcpy(buf, buf + width, width*(height-1));
          memset(buf + (width*(height-1)), 0, width);
          y = height-1;
        }
        buf[y * width + x] = 30;
      }
    }
    */
  }

  //static const int width =64;
  //static const int height=16;

  //char buf[width*height];
  //int  x = 0;
  //int  y = 0;
};

// verilator lint_on unusedparam
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_CONSOLE_H
