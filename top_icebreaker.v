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

  // from icepll for icebreaker
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
      counter <= counter + 1;
    end
  end

  /*
  function logic[6:0] hex_to_ssd(logic[3:0] in);
    case(in)
      4'h0: hex_to_ssd = 7'b1000000;
      4'h1: hex_to_ssd = 7'b1111001;
      4'h2: hex_to_ssd = 7'b0100100;
      4'h3: hex_to_ssd = 7'b0110000;
      4'h4: hex_to_ssd = 7'b0011001;
      4'h5: hex_to_ssd = 7'b0010010;
      4'h6: hex_to_ssd = 7'b0000010;
      4'h7: hex_to_ssd = 7'b1111000;
      4'h8: hex_to_ssd = 7'b0000000;
      4'h9: hex_to_ssd = 7'b0010000;
      4'hA: hex_to_ssd = 7'b0001000;
      4'hB: hex_to_ssd = 7'b0000011;
      4'hC: hex_to_ssd = 7'b1000110;
      4'hD: hex_to_ssd = 7'b0100001;
      4'hE: hex_to_ssd = 7'b0000110;
      4'hF: hex_to_ssd = 7'b0001110;
    endcase
  endfunction

  logic[7:0] out_ssd;

  always_comb begin
    logic[6:0] dig0;
    logic[6:0] dig1;

    LEDR_N = BTN_N;
    LEDG_N = ~BTN_N;

    LED1 = P1B1;
    LED2 = P1B2;
    LED3 = P1B3;
    LED4 = P1B4;
    LED5 = P1B7;

    out_ssd[6:0] = hex_to_ssd(counter[12] ? counter[25:22] : counter[29:26]);
    out_ssd[7]   = counter[12];
  end


  assign P1A1  = out_ssd[0];
  assign P1A2  = out_ssd[1];
  assign P1A3  = out_ssd[2];
  assign P1A4  = out_ssd[3];
  assign P1A7  = out_ssd[4];
  assign P1A8  = out_ssd[5];
  assign P1A9  = out_ssd[6];
  assign P1A10 = out_ssd[7];
  */

endmodule
