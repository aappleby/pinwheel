`include "bram.sv"

/*
module SB_SPRAM256KA (
	input [13:0] ADDRESS,
	input [15:0] DATAIN,
	input [3:0] MASKWREN,
	input WREN, CHIPSELECT, CLOCK, STANDBY, SLEEP, POWEROFF,
	output reg [15:0] DATAOUT
);
*/

//------------------------------------------------------------------------------

module top(
  input  logic      EXT_CLK,
  output logic[7:0] LEDS
);

  //----------------------------------------

  logic clock;
  assign clock = EXT_CLK;

  //----------------------------------------

  logic[31:0] counter;
  logic[2:0]  tick_old;
  logic[2:0]  tick_new;

  initial begin
    counter = 0;
    tick_old = 0;
  end



  always_ff @(posedge clock) begin
    if (counter == 12000000 - 1) begin
      counter <= 0;
      tick <= tick + 1;
    end else begin
      counter <= counter + 1;
    end
  end

  //----------------------------------------

  //----------------------------------------

  always_comb begin

    logic[2:0] tick_old = tick;
    logic[2:0] tick_new

    if (counter == 12000000 - 1) begin
      case (tick)
      0: LEDS = 8'b00000001;
      1: LEDS = 8'b00000010;
      2: LEDS = 8'b00000100;
      3: LEDS = 8'b00001000;
      4: LEDS = 8'b00010000;
      5: LEDS = 8'b00100000;
      6: LEDS = 8'b01000000;
      7: LEDS = 8'b10000000;
      endcase
    end

  end

  //----------------------------------------

endmodule
