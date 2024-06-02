`default_nettype none

`include "SB_PLL40_CORE.v"

module top(
  input  logic EXT_CLK,
  //output logic [7:0] LEDS,

  input logic  BTN_N,
  output logic LEDR_N,
  output logic LEDG_N,

  output logic P1A1,
  output logic P1A2,
  output logic P1A3,
  output logic P1A4,
  output logic P1A7,
  output logic P1A8,
  output logic P1A9,
  output logic P1A10,

  input logic P1B1,
  input logic P1B2,
  input logic P1B3,
  input logic P1B4,
  input logic P1B7,
  input logic P1B8,
  input logic P1B9,
  input logic P1B10,

  input logic  BTN1,
  input logic  BTN2,
  input logic  BTN3,
  output logic LED1,
  output logic LED2,
  output logic LED3,
  output logic LED5,
  output logic LED4,

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
   /*
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
  */

  // for icebreaker
  SB_PLL40_PAD #(
		.FEEDBACK_PATH("SIMPLE"),
		.DIVR(4'b0000),		// DIVR =  0
		.DIVF(7'b0111111),	// DIVF = 63
		.DIVQ(3'b101),		// DIVQ =  5
		.FILTER_RANGE(3'b001)	// FILTER_RANGE = 1
	) uut (
		.LOCK(pll_lock),
		.RESETB(1'b1),
		.BYPASS(1'b0),
		.PACKAGEPIN(EXT_CLK),
		.PLLOUTCORE(clk)
  );


  logic rst;
  assign rst = !pll_lock;

  logic[31:0] counter;
  logic[31:0] seconds;

  always @(posedge clk) begin
    if (rst) begin
      counter <= 0;
      seconds <= 0;
    end else begin
      /*
      if (counter == pll_clock_rate - 1) begin
        counter <= 0;
        seconds <= seconds + 1;
      end else begin
        counter <= counter + 1;
      end
      */
      counter <= counter + 1;
    end
  end

  logic[6:0] SS_0 = 7'b1000000;
  logic[6:0] SS_1 = 7'b1111001;
  logic[6:0] SS_2 = 7'b0100100;
  logic[6:0] SS_3 = 7'b0110000;
  logic[6:0] SS_4 = 7'b0011001;
  logic[6:0] SS_5 = 7'b1111111;
  logic[6:0] SS_6 = 7'b1111111;
  logic[6:0] SS_7 = 7'b1111111;
  logic[6:0] SS_8 = 7'b1111111;
  logic[6:0] SS_9 = 7'b1111111;
  logic[6:0] SS_A = 7'b1111111;
  logic[6:0] SS_B = 7'b1111111;
  logic[6:0] SS_C = 7'b1111111;
  logic[6:0] SS_D = 7'b1111111;
  logic[6:0] SS_E = 7'b1111111;
  logic[6:0] SS_F = 7'b1111111;

  assign LEDR_N = BTN_N;
  assign LEDG_N = ~BTN_N;

  assign LED1 = P1B1;
  assign LED2 = P1B2;
  assign LED3 = P1B3;
  assign LED4 = P1B4;
  assign LED5 = P1B7;

  //assign P1A1  = counter[16];
  //assign P1A2  = counter[17];
  //assign P1A3  = counter[18];
  //assign P1A4  = counter[19];
  //assign P1A7  = counter[20];
  //assign P1A8  = counter[21];
  //assign P1A9  = counter[22];
  //assign P1A10 = counter[23];

  assign P1A1  = SS_4[0];
  assign P1A2  = SS_4[1];
  assign P1A3  = SS_4[2];
  assign P1A4  = SS_4[3];
  assign P1A7  = SS_4[4];
  assign P1A8  = SS_4[5];
  assign P1A9  = SS_4[6];
  assign P1A10 = counter[24];

endmodule
