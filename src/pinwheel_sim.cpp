#include "pinwheel_sim.h"


PinwheelSim::PinwheelSim() : states(new Pinwheel()) {
  auto& pinwheel = states.top();
  memset(pinwheel.console_buf, 0, sizeof(pinwheel.console_buf));
}

bool PinwheelSim::busy() const {
  return steps > 0;
}

void PinwheelSim::step() {
  if (steps) {
    auto& pinwheel = states.top();
    pinwheel.tick(0);

    logic<32> addr = pinwheel.addr_gen(pinwheel.vane1_insn, pinwheel.regs.out_a);
    logic<4>  mask = pinwheel.mask_gen(pinwheel.vane1_insn, addr);

    if ((addr == 0x40000000) && (mask & 1) && (pinwheel.vane1_hart == 0)) {
      pinwheel.console_buf[pinwheel.console_y * 80 + pinwheel.console_x] = 0;

      auto c = char(pinwheel.regs.out_b);

      if (c == '\n') {
        pinwheel.console_x = 0;
        pinwheel.console_y++;
      }
      else if (c == '\r') {
        pinwheel.console_x = 0;
      }
      else {
        pinwheel.console_buf[pinwheel.console_y * 80 + pinwheel.console_x] = c;
        pinwheel.console_x++;
      }

      if (pinwheel.console_x == 80) {
        pinwheel.console_x = 0;
        pinwheel.console_y++;
      }
      if (pinwheel.console_y == 25) {
        memcpy(pinwheel.console_buf, pinwheel.console_buf + 80, 80*24);
        memset(pinwheel.console_buf + (80*24), 0, 80);
        pinwheel.console_y = 24;
      }
      pinwheel.console_buf[pinwheel.console_y * 80 + pinwheel.console_x] = 30;
    }
    steps--;
  }
}
