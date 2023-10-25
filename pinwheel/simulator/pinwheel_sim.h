#include "pinwheel/metron/pinwheel.h"
#include "pinwheel/simulator/sim_thread.h"

#include "metrolib/core/StateStack.h"
#include "metrolib/core/Log.h"
#include "pinwheel/metron/console.h"

struct PinwheelSim : public Sim {

  PinwheelSim(const char* text_file = nullptr, const char* data_file = nullptr);

  virtual bool busy() const override;
  virtual void step() override;

  StatePointerStack<pinwheel> states;
  int64_t steps = 0;
  int64_t ticks = 0;

  Console console1;
  Console console2;
  Console console3;
  Console console4;
};
