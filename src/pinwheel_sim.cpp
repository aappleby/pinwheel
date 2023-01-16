#include "pinwheel_sim.h"


PinwheelSim::PinwheelSim(const char* text_file, const char* data_file) : states(new pinwheel(text_file, data_file)) {
}

bool PinwheelSim::busy() const {
  return steps > 0;
}

void PinwheelSim::step() {
  if (steps) {
    auto& pinwheel = states.top();
    pinwheel.tock(0);
    pinwheel.tick(0);
    steps--;
  }
}
