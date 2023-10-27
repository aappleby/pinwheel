`ifndef PINWHEEL_RAM_SV
`define PINWHEEL_RAM_SV
`include "block_ram.sv"

module pinwheel_ram
#(
 parameter size_bytes = 512,
)
(
  input  logic clk,
  input  logic[addr_bits-1:0] raddr,
  output logic[word_bits-1:0] rdata,
  input  logic[addr_bits-1:0] waddr,
  input  logic[word_bits-1:0] wdata,
  input  logic wren,
);

  localparam size_words = size_bytes / 4;
  localparam addr_bits = $clog2(size_bytes / 4);
  localparam word_bits = 32;

  block_ram #(.width(word_bits), .depth(size_words)) data(
    .rclk(clk), .raddr(raddr), .rdata(rdata),
    .wclk(clk), .waddr(waddr), .wdata(wdata), .wren(wren),
  );

endmodule

`endif // PINWHEEL_RAM_SV
