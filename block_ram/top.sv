module top(
  input  logic<1>  clock,
  input  logic<10> raddr,
  input  logic<10> waddr,
  input  logic<32> wdata,
  input  logic<1>  wren,
  output logic<32> out
);

  always @(posedge clock) begin

  end

  logic<32> data[1024];

endmodule
