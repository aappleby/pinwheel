`ifndef PINWHEEL_RAM_SV
`define PINWHEEL_RAM_SV
`include "block_ram.sv"

module pinwheel_ram
(
  input  logic       clk,
  input  logic[9:0]  raddr,
  output logic[31:0] rdata,
  input  logic[9:0]  waddr,
  input  logic[31:0] wdata,
  input  logic       wren,
);

  block_ram #(.addr_width(10), .data_width(32)) data
  (
    .rclk(clk), .raddr(raddr), .rdata(rdata),
    .wclk(clk), .waddr(waddr), .wdata(wdata), .wren(wren),
  );

endmodule

`endif // PINWHEEL_RAM_SV
