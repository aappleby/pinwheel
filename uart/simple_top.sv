`default_nettype none
`timescale 1 ns/1 ns

`include "simple_rx.sv"
`include "simple_tx.sv"
`include "simple_msg.sv"
`include "simple_sink.sv"

//==============================================================================

module simple_top #(parameter clocks_per_bit)
(
  input logic clock,
  input logic reset
);

  logic[7:0] msg_out;
  logic      msg_out_valid;
  logic      msg_out_ready;

  simple_message msg(
    ._clock     (clock),
    ._reset     (reset),
    ._out       (msg_out),
    ._out_valid (msg_out_valid),
    ._out_ready (msg_out_ready)
  );

  logic      tx_out;

  simple_tx #(.clocks_per_bit(clocks_per_bit)) tx(
    ._clock    (clock),
    ._reset    (reset),
    ._in       (msg_out),
    ._in_valid (msg_out_valid),
    ._in_ready (msg_out_ready),
    ._out      (tx_out)
  );

  logic[7:0] rx_out;
  logic      rx_out_valid;
  logic      rx_out_ready;

  simple_rx #(.clocks_per_bit(clocks_per_bit)) rx(
    ._clock     (clock),
    ._reset     (reset),
    ._in        (tx_out),
    ._out       (rx_out),
    ._out_valid (rx_out_valid),
    ._out_ready (rx_out_ready)
  );

  byte_sink sink(
    ._clock    (clock),
    ._reset    (reset),
    ._in       (rx_out),
    ._in_valid (rx_out_valid),
    ._in_ready (rx_out_ready)
  );

endmodule

//==============================================================================
