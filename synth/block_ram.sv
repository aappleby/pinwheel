`ifndef BLOCK_RAM_SV
`define BLOCK_RAM_SV

module block_ram
#(
 parameter filename = "data/zero.hex",
 parameter width = 8,
 parameter depth = 4096
)
(
  input  logic rclk,
  input  logic[addr_bits-1:0] raddr,
  output logic[data_bits-1:0] rdata,

  input  logic wclk,
  input  logic[addr_bits-1:0] waddr,
  input  logic[data_bits-1:0] wdata,
  input  logic wren,
);

  localparam addr_bits = $clog2(depth);
  localparam data_bits = width;

  initial begin
    $readmemh(filename, mem);
  end

  reg [width-1:0] mem [depth-1:0];

  always @(posedge wclk) begin
    if (wren) mem[waddr] <= wdata;
  end

  always @(posedge rclk) begin
    rdata <= mem[raddr];
  end

endmodule

`endif // BLOCK_RAM_SV
