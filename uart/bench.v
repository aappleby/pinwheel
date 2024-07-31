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
  logic[7:0] cursor, cursor_;

  initial begin
    $readmemh("ping.hex", text);
    delay  = delay_max;
    cursor = 0;
  end

  always_comb begin
    if (_reset) begin
      delay_  = delay_max;
      cursor_ = 0;
    end else if (delay) begin
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

  assign _out       = _reset ? 0 : delay ? 0 : text[cursor];
  assign _out_valid = _reset ? 0 : (delay == 0);

endmodule

//==============================================================================

module simple_tx
#(
  parameter clocks_per_bit
)
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

    if (_reset) begin
      tx_delay_ = delay_max;
      tx_count_ = count_max;
      tx_shift_ = 10'b1111111111;
      _in_ready = 0;
    end else begin
      tx_shift_ = tx_shift;
      _in_ready = 0;
      if (tx_delay < delay_max) begin
        tx_delay_ = tx_delay + 1;
        tx_count_ = tx_count;
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

module simple_rx
#(
  parameter clocks_per_bit
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
  localparam delay_max = clocks_per_bit - 1;
  localparam count_max = bits_per_byte - 1; // 1 start + 8 data + 9 stop to make sure we're really stopped :D

  logic[7:0] delay_, delay = delay_max;
  logic[7:0] count_, count = count_max;
  logic[7:0] shift_, shift = 0;

  //----------------------------------------

  /*
  if delay < delay_max
    @delay = delay + 1
  else if count < count_max
    @delay = 0
    @count = count + 1
  else if ser_in == 0
    @delay = 0
    @count = 0

  if delay == delay_mid
    @shift = (ser_in :: shift) >> 1
    valid = count == 8
  */

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
      if (delay < delay_max) begin
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

  wire _rx_bit = delay == delay_mid && count > 0 && count < count_max;

endmodule

//==============================================================================

module byte_sink
(
  input logic      _clock,
  input logic      _reset,

  input logic[7:0] _data,
  input logic      _data_valid,
  output logic     _data_ready
);

  logic[7:0] data = 0;

  always_comb begin
    _data_ready = !_reset;
  end

  always @(posedge _clock) begin
    if (_data_valid) begin
      data <= _data;
    end
  end

endmodule

//==============================================================================

module bench();

  logic clock = 1;
  logic reset = 1;

  always #5 clock = ~clock;

  initial begin
    $dumpfile ("bench.vcd");
    $dumpvars;

    #95;
    reset = 0;

    #8000 $finish();
  end

  //----------------------------------------------------------------------------

  localparam clocks_per_bit = 1;

  simple_message msg(
    ._clock     (clock),
    ._reset     (reset),
    ._out_ready (tx._in_ready)
  );

  simple_tx #(.clocks_per_bit(clocks_per_bit)) tx(
    ._clock    (clock),
    ._reset    (reset),
    ._in       (msg._out),
    ._in_valid (msg._out_valid)
  );

  simple_rx #(.clocks_per_bit(clocks_per_bit)) rx(
    ._clock     (clock),
    ._reset     (reset),
    ._in        (tx._out),
    ._out_ready (sink._data_ready)
  );

  byte_sink sink(
    ._clock      (clock),
    ._reset      (reset),
    ._data       (rx._out),
    ._data_valid (rx._out_valid)
  );

  //----------------------------------------------------------------------------

endmodule

//==============================================================================
