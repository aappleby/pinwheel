`ifndef UART_BENCH_SV
`define UART_BENCH_SV
`default_nettype none

`include "uart_top.sv"
`include "SB_PLL40_CORE.v"

//==============================================================================

module uart_ice40(
  input  logic EXT_CLK,

  // Serial port to host
  output logic SER_DCDn,
  output logic SER_DSRn,
  input  logic SER_DTRn,
  input  logic SER_RTSn,
  output logic SER_CTSn,
  output logic SER_TX,
  input  logic SER_RX,

  // On-board LEDs
  output logic [7:0] LEDS,

  // Top pin row connection to logic analyser
  output logic [7:0] DEBUG
);

  logic pll_clock;
  logic pll_lock;

  /**
   * PLL configuration
   *
   * This Verilog module was generated automatically
   * using the icepll tool from the IceStorm project.
   * Use at your own risk.
   *
   * Given input frequency:        12.000 MHz
   * Requested output frequency:   24.000 MHz
   * Achieved output frequency:    24.000 MHz
   */
  SB_PLL40_CORE #(
                  .FEEDBACK_PATH("SIMPLE"),
                  .DIVR(4'b0000),         // DIVR =  0
                  .DIVF(7'b0111111),      // DIVF = 63
                  .DIVQ(3'b101),          // DIVQ =  5
                  .FILTER_RANGE(3'b001)   // FILTER_RANGE = 1
          ) uut (
                  .LOCK(pll_lock),
                  .RESETB(1'b1),
                  .BYPASS(1'b0),
                  .REFERENCECLK(EXT_CLK),
                  .PLLOUTCORE(pll_clock)
                  );


  localparam pll_clock_rate = 24000000;
  localparam ser_clock_rate = 1200;
  localparam cycles_per_bit = pll_clock_rate / ser_clock_rate;

  logic o_serial;
  logic o_valid;
  logic[7:0] o_data;
  logic o_done;
  logic[31:0] o_sum;
  logic reset;

  assign reset = !pll_lock;

  uart_top #(.cycles_per_bit(cycles_per_bit), .repeat_msg(1)) dut
  (
    .clock(pll_clock),
    .get_serial_ret(o_serial),
    .get_valid_ret(o_valid),
    .get_data_out_ret(o_data),
    .get_done_ret(o_done),
    .get_checksum_ret(o_sum),
    .tock_reset(reset)
  );

  always_comb begin
    SER_TX = o_serial;
    DEBUG[0] = o_serial;
    DEBUG[1] = o_serial;
    DEBUG[2] = o_serial;
    DEBUG[3] = o_serial;
    DEBUG[4] = o_serial;
    DEBUG[5] = o_serial;
    DEBUG[6] = o_serial;
    DEBUG[7] = o_serial;
    LEDS = o_valid ? o_data : 0;
  end

endmodule

//==============================================================================

`endif
