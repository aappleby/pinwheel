#include "pinwheel/soc/pinwheel_soc.h"

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
    "pinwheel/uart/message.hex",
    /*int clock_rate =*/ 12000000,
    /*int baud_rate =*/ 1000000,
    /*int repeat_msg =*/ 0
  );


  top.tock(1);

  LOG_B("========================================\n");

  //1688703

  for (int cycle = 0; cycle < 1688703; cycle++) {
    bool old_valid = top.uart0_rx.get_valid();
    top.tock(0);
    if (!old_valid && top.uart0_rx.get_valid()) {
      LOG_B("%c", (uint8_t)top.uart0_rx.get_data_out());
    }

    if (top.uart0_hello.get_done() && top.uart0_tx.get_idle()) {
      break;
    }
  }

  LOG_B("\n");
  LOG_B("========================================\n");

  EXPECT_EQ(0x0000b764, top.uart0_rx.get_checksum(), "Verilator uart checksum fail");

  TEST_DONE();
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  TestResults results;
  results << test_uart_metron();
  return results.show_result();
}

//------------------------------------------------------------------------------
