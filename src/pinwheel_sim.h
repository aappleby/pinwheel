#include "pinwheel.h"
#include "sim_thread.h"

#include "CoreLib/StateStack.h"
#include "CoreLib/Log.h"

struct PinwheelSim : public Sim {

  PinwheelSim();

  virtual bool busy() const override;
  virtual void step() override;

  StatePointerStack<pinwheel> states;
  int steps = 0;
};
