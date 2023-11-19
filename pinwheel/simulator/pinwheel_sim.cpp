#include "pinwheel_sim.h"

//------------------------------------------------------------------------------

PinwheelSim::PinwheelSim(const char* code_hex, const char* data_hex, const char* message_hex)
: states(new PinwheelTB(code_hex, data_hex, message_hex))
{
}

//------------------------------------------------------------------------------

bool PinwheelSim::busy() const {
  return steps > 0;
}

//------------------------------------------------------------------------------

void PinwheelSim::step() {
  if (steps) {
    auto& pinwheel = states.top();

    auto old_uart_valid = pinwheel.soc.uart0_rx.get_valid();

    pinwheel.soc.tock(0);
    pinwheel.console1.tick(0, pinwheel.soc.core.data_tla);
    pinwheel.console2.tick(0, pinwheel.soc.core.data_tla);
    pinwheel.console3.tick(0, pinwheel.soc.core.data_tla);
    pinwheel.console4.tick(0, pinwheel.soc.core.data_tla);

    if (!old_uart_valid && pinwheel.soc.uart0_rx.get_valid()) {
      pinwheel.console5.putchar(pinwheel.soc.uart0_rx.get_data_out());
    }

    steps--;
    ticks++;
  }
}

//------------------------------------------------------------------------------
