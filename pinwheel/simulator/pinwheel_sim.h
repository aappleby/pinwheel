#include "metrolib/core/Log.h"
#include "metrolib/core/StateStack.h"
#include "pinwheel/simulator/sim_thread.h"
#include "pinwheel/soc/console.h"
#include "pinwheel/soc/pinwheel_soc.h"

struct PinwheelSim : public Sim {

  PinwheelSim(const char* code_hex, const char* data_hex, const char* message_hex);

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
