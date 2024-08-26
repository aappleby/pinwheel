`default_nettype none
`timescale 1 ns/1 ns

//==============================================================================

module simple_message
(
  input  logic      _clock,
  input  logic      _reset,

  output logic[7:0] _out,
  output logic      _out_valid,
  input  logic      _out_ready
);

  localparam delay_max = 20;

  logic[7:0] text[0:5];
  logic[7:0] delay, delay_;
  logic[2:0] cursor, cursor_;

  initial begin
    $readmemh("ping.hex", text);
    delay  = delay_max;
    cursor = 0;
  end

  always_comb begin
    if (_reset) begin
      delay_  = delay_max;
      cursor_ = 0;
    end else if (delay > 0) begin
      delay_  = delay - 1;
      cursor_ = cursor;
    end else if (_out_valid && _out_ready) begin
      delay_  = cursor == 5 ? delay_max - 1 : delay;
      cursor_ = cursor == 5 ? 0 : cursor + 1;
    end else begin
      delay_  = delay;
      cursor_ = cursor;
    end
  end

  always @(posedge _clock) begin
    delay  <= delay_;
    cursor <= cursor_;
  end

  assign _out       = _reset ? 0 : delay > 0 ? 0 : text[cursor];
  assign _out_valid = _reset ? 0 : (delay == 0);

endmodule
