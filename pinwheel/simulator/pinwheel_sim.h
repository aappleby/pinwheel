#include "metrolib/core/Log.h"
#include "metrolib/core/StateStack.h"
#include "pinwheel/simulator/sim_thread.h"
#include "pinwheel/soc/console.h"
#include "pinwheel/soc/pinwheel_soc.h"

//------------------------------------------------------------------------------

struct PinwheelTB {

  PinwheelTB(
    const char* code_hexfile = "pinwheel/tools/blank.code.vh",
    const char* data_hexfile = "pinwheel/tools/blank.data.vh",
    const char* message_hex  = "pinwheel/uart/message.hex")
  : soc(code_hexfile, data_hexfile, message_hex)
  {
    console1.init(0xF0000000, 0x40000000);
    console2.init(0xF0000000, 0x50000000);
    console3.init(0xF0000000, 0x60000000);
    console4.init(0xF0000000, 0x70000000);
    console5.init(0x00000000, 0x00000000);
  }

  PinwheelTB* clone() {
    PinwheelTB* p = new PinwheelTB(nullptr, nullptr, nullptr);
    memcpy(p, this, sizeof(*this));
    return p;
  }

  size_t size_bytes() { return sizeof(*this); }

  pinwheel_soc soc;
  Console console1;
  Console console2;
  Console console3;
  Console console4;
  Console console5;
};

//------------------------------------------------------------------------------

struct PinwheelSim : public Sim {

  PinwheelSim(const char* code_hex, const char* data_hex, const char* message_hex);
  virtual ~PinwheelSim() {}

  virtual bool busy() const override;
  virtual void step() override;

  StatePointerStack<PinwheelTB> states;
  int64_t steps = 0;
  int64_t ticks = 0;
};

//------------------------------------------------------------------------------
