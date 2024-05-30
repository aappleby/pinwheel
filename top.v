`default_nettype none

module top(
  input  logic EXT_CLK,
  output logic [7:0] LEDS,
);

  assign LEDS[0] = 1'b0;
  assign LEDS[1] = 1'b0;
  assign LEDS[2] = 1'b1;
  assign LEDS[3] = 1'b1;
  assign LEDS[4] = 1'b0;
  assign LEDS[5] = 1'b1;
  assign LEDS[6] = 1'b1;
  assign LEDS[7] = 1'b0;

endmodule
