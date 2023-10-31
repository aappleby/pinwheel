`ifndef TOP_ICE40_SV
`define TOP_ICE40_SV
`default_nettype none

`include "ice40/SB_PLL40_CORE.v"

//`include "pinwheel/soc/pinwheel_soc.sv"

//==============================================================================

module pinwheel_ice40(
  input  logic EXT_CLK,
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

  initial begin
    int i;

    SER_DCDn = 1;
    SER_DSRn = 1;
    SER_CTSn = 1;
    SER_TX = 1;

    for (i = 0; i < 8; i++) LEDS[i]  = 0;
    for (i = 0; i < 8; i++) DEBUG[i] = 0;
  end

  logic CLK;
  logic pll_lock;
  localparam pll_clock_rate = 24000000;
  localparam ser_clock_rate = 1200;

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
  )
  uut (
    .LOCK(pll_lock),
    .RESETB(1'b1),
    .BYPASS(1'b0),
    .REFERENCECLK(EXT_CLK),
    .PLLOUTCORE(CLK)
  );


  logic reset;
  assign reset = !pll_lock;

  //assign LEDS[0] = rdata[0];
  //assign LEDS[1] = rdata[1];
  //assign LEDS[2] = rdata[2];
  //assign LEDS[3] = rdata[3];
  //assign LEDS[4] = rdata[4];
  //assign LEDS[5] = rdata[5];
  //assign LEDS[6] = rdata[6];
  //assign LEDS[7] = rdata[7];

endmodule

//==============================================================================

`endif // TOP_ICE40_SV
