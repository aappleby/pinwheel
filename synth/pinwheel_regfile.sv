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


  block_ram regs_lo_0(
    .rclk(clk), .raddr(raddr0), .rdata(rdata0[15:0]),
    .wclk(clk), .waddr(waddr),  .wdata(wdata [15:0]), .wren(wren),
  );

  block_ram regs_lo_1(
    .rclk(clk), .raddr(raddr1), .rdata(rdata1[15:0]),
    .wclk(clk), .waddr(waddr),  .wdata(wdata [15:0]), .wren(wren),
  );

  block_ram regs_hi_0(
    .rclk(clk), .raddr(raddr0), .rdata(rdata0[31:16]),
    .wclk(clk), .waddr(waddr),  .wdata(wdata [31:16]), .wren(wren),
  );

  block_ram regs_hi_1(
    .rclk(clk), .raddr(raddr1), .rdata(rdata1[31:16]),
    .wclk(clk), .waddr(waddr),  .wdata(wdata [31:16]), .wren(wren),
  );


endmodule


`endif // PINWHEEL_REGFILE_SV
