#include "pinwheel/metron/pinwheel.h"
#include "pinwheel/simulator/sim_thread.h"

#include "metrolib/core/StateStack.h"
#include "metrolib/core/Log.h"

struct PinwheelSim : public Sim {

  PinwheelSim(const char* text_file = nullptr, const char* data_file = nullptr);

  virtual bool busy() const override;
  virtual void step() override;

  StatePointerStack<pinwheel> states;
  int steps = 0;
};
