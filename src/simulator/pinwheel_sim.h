#include "pinwheel.h"
#include "sim_thread.h"

#include "metrolib/src/CoreLib/StateStack.h"
#include "metrolib/src/CoreLib/Log.h"

struct PinwheelSim : public Sim {

  PinwheelSim(const char* text_file = nullptr, const char* data_file = nullptr);

  virtual bool busy() const override;
  virtual void step() override;

  StatePointerStack<pinwheel> states;
  int steps = 0;
};
