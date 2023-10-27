`ifndef PINWHEEL_REGFILE_SV
`define PINWHEEL_REGFILE_SV
`include "block_ram.sv"

module pinwheel_regfile
(
  input  logic       clk,
  input  logic[7:0]  raddr0,
  output logic[31:0] rdata0,
  input  logic[7:0]  raddr1,
  output logic[31:0] rdata1,
  input  logic[7:0]  waddr,
  input  logic[31:0] wdata,
  input  logic       wren,
);

  block_ram #(.width(32), .depth(32*4)) regs0(
    .rclk(clk), .raddr(raddr0), .rdata(rdata0),
    .wclk(clk), .waddr(waddr),  .wdata(wdata ), .wren(wren),
  );

  block_ram #(.width(32), .depth(32*4)) regs1(
    .rclk(clk), .raddr(raddr1), .rdata(rdata1),
    .wclk(clk), .waddr(waddr),  .wdata(wdata ), .wren(wren),
  );

endmodule

`endif // PINWHEEL_REGFILE_SV
