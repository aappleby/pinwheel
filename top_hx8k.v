`default_nettype none

`include "SB_PLL40_CORE.v"

module top(
  input  logic EXT_CLK,
  output logic [7:0] LEDS,
);

  logic clk;
  logic pll_lock;
  localparam pll_clock_rate = 24000000;

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
    .PLLOUTCORE(clk)
  );

  logic rst;
  assign rst = !pll_lock;

  logic[31:0] counter;

  always @(posedge clk) begin
    if (rst) begin
      counter <= 0;
    end else begin
      counter <= counter + 1;
    end
  end

  assign LEDS = counter[30:23];

endmodule
