`default_nettype none
`timescale 1 ns/1 ns

//==============================================================================

module simple_rx
#(
  parameter logic[7:0] clocks_per_bit
)
(
  input logic       _clock,
  input logic       _reset,

  input logic       _in,

  output logic[7:0] _out,
  output logic      _out_valid,
  input  logic      _out_ready
);

  localparam bits_per_byte = 10; // 1 start + 8 data + 1 stop
  localparam delay_mid = clocks_per_bit / 2;
  localparam logic[7:0] delay_max = clocks_per_bit - 1;
  localparam count_max = bits_per_byte - 1; // 1 start + 8 data + 9 stop to make sure we're really stopped :D

  logic[7:0] delay_, delay = delay_max;
  logic[7:0] count_, count = count_max;
  logic[7:0] shift_, shift = 0;

  //----------------------------------------

  always_comb begin
    delay_ = delay;
    count_ = count;
    shift_ = shift;
    _out = 0;
    _out_valid = 0;

    if (_reset) begin
      delay_ = delay_max;
      count_ = count_max;
      shift_ = 0;
    end else begin
      if (delay != delay_max) begin
        delay_ = delay + 1;
      end else if (count < count_max) begin
        delay_ = 0;
        count_ = count + 1;
      end else if (_in == 0) begin
        delay_ = 0;
        count_ = 0;
      end

      if (delay_ == delay_mid) begin
        shift_ = {_in, shift[7:1]};
        if (count_ == count_max - 1) begin
          _out       = shift_;
          _out_valid = 1;
        end
      end
    end
  end

  //----------------------------------------

  always @(posedge _clock) begin
    delay <= delay_;
    count <= count_;
    shift <= shift_;
  end

endmodule

//==============================================================================
