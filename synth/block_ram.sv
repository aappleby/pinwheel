`ifndef BLOCK_RAM_SV
`define BLOCK_RAM_SV

module block_ram
#(
 parameter filename = "data/zero.hex",
 parameter addr_width = 8,
 parameter data_width = 16
)
(
  input logic rclk,
  input logic[addr_width-1:0] raddr,
  output logic[data_width-1:0] rdata,

  input logic wclk,
  input logic[addr_width-1:0] waddr,
  input logic[data_width-1:0] wdata,
  input logic wren,
);

  initial begin
    $readmemh(filename, mem);
  end

  reg [data_width-1:0] mem [(1<<addr_width)-1:0];

  always @(posedge wclk) begin
    if (wren) mem[waddr] <= wdata;
  end

  always @(posedge rclk) begin
    rdata <= mem[raddr];
  end

endmodule

`endif // BLOCK_RAM_SV
