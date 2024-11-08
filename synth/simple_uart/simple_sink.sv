`default_nettype none
`timescale 1 ns/1 ns

//==============================================================================

module byte_sink
(
  input logic      _clock,
  input logic      _reset,

  input logic[7:0] _in,
  input logic      _in_valid,
  output logic     _in_ready
);

  logic[7:0] data = 0;

  always_comb begin
    _in_ready = !_reset;
  end

  always @(posedge _clock) begin
    if (_in_valid) begin
      data <= _in;
    end
  end

endmodule
