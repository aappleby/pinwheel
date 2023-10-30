#include "metrolib/core/Log.h"
#include "metrolib/core/StateStack.h"
#include "pinwheel/simulator/sim_thread.h"
#include "pinwheel/soc/console.h"
#include "pinwheel/soc/pinwheel_soc.h"

//------------------------------------------------------------------------------

struct PinwheelWrapper {

  PinwheelWrapper(
    const char* code_hexfile = "pinwheel/tools/blank.code.vh",
    const char* data_hexfile = "pinwheel/tools/blank.data.vh",
    const char* message_hex  = "pinwheel/uart/message.hex")
  : soc(code_hexfile, data_hexfile, message_hex) {
  }

  PinwheelWrapper* clone() {
    PinwheelWrapper* p = new PinwheelWrapper(nullptr, nullptr, nullptr);
    memcpy(p, this, sizeof(*this));
    return p;
  }

  size_t size_bytes() { return sizeof(*this); }

  pinwheel_soc soc;
};

//------------------------------------------------------------------------------

struct PinwheelSim : public Sim {

  PinwheelSim(const char* code_hex, const char* data_hex, const char* message_hex);

  virtual bool busy() const override;
  virtual void step() override;

  StatePointerStack<PinwheelWrapper> states;
  int64_t steps = 0;
  int64_t ticks = 0;

  Console console1;
  Console console2;
  Console console3;
  Console console4;
};

//------------------------------------------------------------------------------
