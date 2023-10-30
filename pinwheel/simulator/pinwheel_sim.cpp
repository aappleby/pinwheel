#include "pinwheel_sim.h"

//------------------------------------------------------------------------------

PinwheelSim::PinwheelSim(const char* text_file, const char* data_file)
: states(new pinwheel(text_file, data_file, "pinwheel/uart/message.hex")),
  console1(0xF0000000, 0x40000000),
  console2(0xF0000000, 0x50000000),
  console3(0xF0000000, 0x60000000),
  console4(0xF0000000, 0x70000000)
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
    pinwheel.tock(0, 0, 0);
    pinwheel.tick(0, 0, 0);
    console1.tick(0, pinwheel.core.bus_tla);
    console2.tick(0, pinwheel.core.bus_tla);
    console3.tick(0, pinwheel.core.bus_tla);
    console4.tick(0, pinwheel.core.bus_tla);
    steps--;
    ticks++;
  }
}

//------------------------------------------------------------------------------
