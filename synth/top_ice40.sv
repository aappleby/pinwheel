`ifndef TOP_ICE40_SV
`define TOP_ICE40_SV
`default_nettype none

`include "SB_PLL40_CORE.v"

//==============================================================================

module uart_ice40(
  input logic CLK,

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
  output logic LOGIC7
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
                  .REFERENCECLK(CLK),
                  .PLLOUTCORE(pll_clock)
                  );


  localparam pll_clock_rate = 24000000;
  localparam ser_clock_rate = 1200;

  logic reset;
  assign reset = !pll_lock;

  assign LEDS[0] = 1b1;
  assign LEDS[1] = 0b1;
  assign LEDS[2] = 1b1;
  assign LEDS[3] = 0b1;
  assign LEDS[4] = 1b1;
  assign LEDS[5] = 1b1;
  assign LEDS[6] = 0b1;
  assign LEDS[7] = 1b1;

endmodule

//==============================================================================

`endif // TOP_ICE40_SV
