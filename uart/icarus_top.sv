`default_nettype none
`timescale 1 ns/1 ns

`include "simple_top.sv"

//==============================================================================

module icarus_top();

  logic clock = 1;
  logic reset = 1;

  always #5 clock = ~clock;
  initial #95 reset = 0;

  initial begin
    $dumpfile ("icarus_top.vcd");
    $dumpvars;
    #8000 $finish();
  end

  simple_top #(.clocks_per_bit(3)) top(
    .clock(clock),
    .reset(reset)
  );

endmodule

//==============================================================================
