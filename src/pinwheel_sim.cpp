#include "pinwheel_sim.h"


PinwheelSim::PinwheelSim() : states(new Pinwheel()) {
  auto& pinwheel = states.top();
}

bool PinwheelSim::busy() const {
  return steps > 0;
}

void PinwheelSim::step() {
  if (steps) {
    auto& pinwheel = states.top();
    pinwheel.tock_twocycle(0);
    pinwheel.tick_twocycle(0);
    steps--;
  }
}
