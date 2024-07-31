`default_nettype none
`timescale 1 ns/1 ns

//==============================================================================

module simple_message
(
  input  logic      _clock,
  input  logic      _reset,
  output logic[7:0] _out,
  output logic      _valid,
  input  logic      _ready
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
    end else if (_valid && _ready) begin
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

  assign _out   = _reset ? 0 : delay ? 0 : text[cursor];
  assign _valid = _reset ? 0 : (delay == 0);

endmodule

//==============================================================================

module simple_tx
#(
  parameter clocks_per_bit
)
(
  input logic      _clock,
  input logic      _reset,

  input logic[7:0] _tx_in,
  input logic      _source_valid,

  output logic     _sink_ready,
  output logic     _tx_out
);

  localparam bits_per_byte = 10; // 1 start + 8 data + 1 stop
  localparam delay_mid = clocks_per_bit / 2;
  localparam delay_max = clocks_per_bit - 1;
  localparam count_max = bits_per_byte - 1; // 1 start + 8 data + 9 stop to make sure we're really stopped :D

  logic[7:0] tx_delay, tx_delay_;
  logic[7:0] tx_count, tx_count_;
  logic[9:0] tx_shift, tx_shift_;

  initial begin
    tx_delay = delay_max;
    tx_count = count_max;
    tx_shift = 10'b1111111111;
  end

  assign _sink_ready = !_reset && tx_delay == delay_max && tx_count == count_max;
  assign _tx_out   = tx_shift[0];

  wire _tx_bit   = tx_delay == 0 && tx_count > 0 && tx_count < count_max;
  wire _tx_byte  = tx_delay == 0 && tx_count == 1;

  always_comb begin
    if (_reset) begin
      tx_delay_ = delay_max;
      tx_count_ = count_max;
    end else if (tx_delay < delay_max) begin
      tx_delay_ = tx_delay + 1;
      tx_count_ = tx_count;
    end else if (tx_count < count_max) begin
      tx_delay_ = 0;
      tx_count_ = tx_count + 1;
    end else if (_source_valid && _sink_ready) begin
      tx_delay_ = 0;
      tx_count_ = 0;
    end
  end

  always_comb begin
    if (_reset) begin
      tx_shift_ = 10'b1111111111;
    end else if (_source_valid && _sink_ready) begin
      tx_shift_ = { 1'b1, _tx_in, 1'b0 };
    end else if (tx_delay_ == 0) begin
      tx_shift_ = { 1'b1, tx_shift[9:1] };
    end else begin
      tx_shift_ = tx_shift;
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
  output logic      _source_valid,
  input  logic      _sink_ready
);

  localparam bits_per_byte = 10; // 1 start + 8 data + 1 stop
  localparam delay_mid = clocks_per_bit / 2;
  localparam delay_max = clocks_per_bit - 1;
  localparam count_max = bits_per_byte - 1; // 1 start + 8 data + 9 stop to make sure we're really stopped :D

  logic[7:0] rx_delay, rx_delay_;
  logic[7:0] rx_count, rx_count_;

  logic[7:0] rx_shift, rx_shift_;
  logic[7:0] rx_data,  rx_data_;
  logic[7:0] rx_valid, rx_valid_;

  logic[31:0] rx_sum, rx_sum_;

  initial begin
    rx_delay = delay_max;
    rx_count = count_max;
    rx_shift = 0;
    rx_data  = 0;
    rx_sum   = 0;
  end

  assign _source_valid = _reset ? 0 : rx_delay == delay_mid && rx_count == count_max - 1;
  assign _out   = rx_data;

  //----------------------------------------

  always_comb begin
    if (_reset) begin
      rx_delay_ = delay_max;
      rx_count_ = count_max;
    end else if (rx_delay < delay_max) begin
      rx_delay_ = rx_delay + 1;
    end else if (rx_count < count_max) begin
      rx_delay_ = 0;
      rx_count_ = rx_count + 1;
    end else if (_in == 0) begin
      rx_delay_ = 0;
      rx_count_ = 0;
    end else begin
      rx_delay_ = rx_delay;
      rx_count_ = rx_count;
    end
  end

  //----------------------------------------

  always_comb begin
    if (_reset) begin
      rx_data_  = 0;
      rx_valid_ = 0;
    end else if (rx_delay_ == delay_mid && rx_count_ == count_max - 1) begin
      rx_data_  = rx_shift_;
      rx_sum_   = rx_sum + rx_shift_;
      rx_valid_ = 1;
    end else begin
      rx_data_  = rx_data;
      rx_valid_ = rx_valid;
    end
  end

  //----------------------------------------

  always_comb begin
    if (_reset) begin
      rx_shift_ = 0;
    end else if (rx_delay_ == delay_mid) begin
      rx_shift_ = {_in, rx_shift[7:1]};
    end else begin
      rx_shift_ = rx_shift;
    end
  end

  //----------------------------------------

  always @(posedge _clock) begin
    rx_data  <= rx_data_;
    rx_delay <= rx_delay_;
    rx_count <= rx_count_;
    rx_shift <= rx_shift_;
    rx_sum   <= rx_sum_;
  end

  wire _rx_bit = rx_delay == delay_mid && rx_count > 0 && rx_count < count_max;

endmodule

//==============================================================================

module byte_sink
(
  input logic      _clock,
  input logic      _reset,
  input logic[7:0] _data,
  input logic      _source_valid,
  output logic     _sink_ready
);

  always_comb begin
    _sink_ready = !_reset;
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

  simple_message msg();

  assign msg._clock = clock;
  assign msg._reset = reset;
  assign msg._ready = tx._sink_ready;

  simple_tx #(.clocks_per_bit(clocks_per_bit)) tx();

  assign tx._clock    = clock;
  assign tx._reset    = reset;
  assign tx._tx_in    = msg._out;
  assign tx._source_valid = msg._valid;

  simple_rx #(.clocks_per_bit(clocks_per_bit)) rx();

  assign rx._clock = clock;
  assign rx._reset = reset;
  assign rx._in = tx._tx_out;
  assign rx._sink_ready = sink._sink_ready;

  byte_sink sink();
  assign sink._clock = clock;
  assign sink._reset = reset;
  assign sink._data  = rx._out;
  assign sink._source_valid = rx._source_valid;

  //----------------------------------------------------------------------------

endmodule

//==============================================================================
