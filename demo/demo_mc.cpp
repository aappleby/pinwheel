#include "pinwheel/soc/pinwheel_soc.h"
#include "pinwheel/uart/uart_top.h"

#include <stdio.h>

#include "metrolib/core/Platform.h"
#include "metrolib/core/Utils.h"
#include "metrolib/core/Tests.h"

//------------------------------------------------------------------------------

TestResults test_uart_metron() {
  TEST_INIT("Metron UART simulation\n");

  pinwheel_soc top(
    "gen/tests/firmware/hello.code.vh",
    "gen/tests/firmware/hello.data.vh",
    "pinwheel/uart/message.hex"
  );


  top.tock(1);

  LOG_B("========================================\n");

  for (int cycle = 0; cycle < 20000; cycle++) {
    bool old_valid = top.rxi.get_valid();
    top.tock(0);
    if (!old_valid && top.rxi.get_valid()) {
      LOG_B("%c", (uint8_t)top.rxi.get_data_out());
    }

    if (top.hello.get_done() && top.tx.get_idle()) {
      break;
    }
  }

  LOG_B("\n");
  LOG_B("========================================\n");

  EXPECT_EQ(0x0000b764, top.rxi.get_checksum(), "Verilator uart checksum fail");

  TEST_DONE();
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  TestResults results;
  results << test_uart_metron();
  return results.show_result();
}

//------------------------------------------------------------------------------
