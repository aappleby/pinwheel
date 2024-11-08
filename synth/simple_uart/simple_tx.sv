`default_nettype none
`timescale 1 ns/1 ns

//==============================================================================

module simple_tx #(parameter clocks_per_bit)
(
  input logic      _clock,
  input logic      _reset,

  input logic[7:0] _in,
  input logic      _in_valid,
  output logic     _in_ready,

  output logic     _out
);

  localparam bits_per_byte = 10; // 1 start + 8 data + 1 stop
  localparam delay_mid = clocks_per_bit / 2;
  localparam delay_max = clocks_per_bit - 1;
  localparam count_max = bits_per_byte - 1; // 1 start + 8 data + 9 stop to make sure we're really stopped :D

  logic[7:0] tx_delay_, tx_delay = delay_max;
  logic[7:0] tx_count_, tx_count = count_max;
  logic[9:0] tx_shift_, tx_shift = 10'b1111111111;

  always_comb begin
    _out = tx_shift[0];
    _in_ready = 0;
    tx_delay_ = tx_delay;
    tx_count_ = tx_count;
    tx_shift_ = tx_shift;

    if (_reset) begin
      tx_delay_ = delay_max;
      tx_count_ = count_max;
      tx_shift_ = 10'b1111111111;
    end else begin
      if (tx_delay != delay_max) begin
        tx_delay_ = tx_delay + 1;
      end else if (tx_count < count_max) begin
        tx_delay_ = 0;
        tx_count_ = tx_count + 1;
        tx_shift_ = { 1'b1, tx_shift[9:1] };
      end else begin
        _in_ready = 1;
        if (_in_valid) begin
          tx_delay_ = 0;
          tx_count_ = 0;
          tx_shift_ = { 1'b1, _in, 1'b0 };
        end
      end
    end
  end

  always @(posedge _clock) begin
    tx_delay <= tx_delay_;
    tx_count <= tx_count_;
    tx_shift <= tx_shift_;
  end

endmodule

//==============================================================================
