`ifndef PINWHEEL_REGFILE_SV
`define PINWHEEL_REGFILE_SV
`include "block_ram.sv"

module pinwheel_regfile
#(
  parameter reg_count = 32,
  parameter reg_width = 32,
  parameter thread_count = 4,
)
(
  input  logic clk,
  input  logic[addr_bits-1:0] raddr0,
  output logic[word_bits-1:0] rdata0,
  input  logic[addr_bits-1:0] raddr1,
  output logic[word_bits-1:0] rdata1,
  input  logic[addr_bits-1:0] waddr,
  input  logic[word_bits-1:0] wdata,
  input  logic wren,
);

  localparam reg_total = reg_count * thread_count;
  localparam addr_bits = $clog2(reg_count * thread_count);
  localparam word_bits = reg_width;

  block_ram #(.width(word_bits), .depth(reg_total)) regs0(
    .rclk(clk), .raddr(raddr0), .rdata(rdata0),
    .wclk(clk), .waddr(waddr),  .wdata(wdata ), .wren(wren),
  );

  block_ram #(.width(word_bits), .depth(reg_total)) regs1(
    .rclk(clk), .raddr(raddr1), .rdata(rdata1),
    .wclk(clk), .waddr(waddr),  .wdata(wdata ), .wren(wren),
  );

endmodule

`endif // PINWHEEL_REGFILE_SV
