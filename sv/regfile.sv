`include "metron_tools.sv"

`include "constants.sv"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal

module regfile (
  // global clock
  input logic clock,
  // tock() ports
  input logic[9:0] tock_raddr1_,
  input logic[9:0] tock_raddr2_,
  input logic[9:0] tock_waddr_,
  input logic[31:0] tock_wdata_,
  input logic tock_wren_,
  // get_rs1() ports
  output logic[31:0] get_rs1_ret,
  // get_rs2() ports
  output logic[31:0] get_rs2_ret
);
/*public:*/

  initial begin
    int i;
    for (i = 0; i < 1024; i = i + 1) data[i] = 0;
  end

  always_comb begin : tock
    raddr1 = tock_raddr1_;
    raddr2 = tock_raddr2_;
    waddr  = tock_waddr_;
    wdata  = tock_wdata_;
    wren   = tock_wren_;
  end

  always_ff @(posedge clock) begin : tick
    out_1 <= data[raddr1];
    out_2 <= data[raddr2];

    if (wren) data[waddr] <= wdata;

    if (wren && raddr1 == waddr) out_1 <= wdata;
    if (wren && raddr2 == waddr) out_2 <= wdata;
  end

  always_comb begin : get_rs1 get_rs1_ret = out_1; end
  always_comb begin : get_rs2 get_rs2_ret = out_2; end

  // metron_noconvert
  /*const uint32_t* get_data() const {
    return (uint32_t*)data;
  }*/

  // metron_internal
  logic[9:0] raddr1;
  logic[9:0] raddr2;
  logic[9:0] waddr;
  logic[31:0] wdata;
  logic  wren;

  logic[31:0] data[1024];
  logic[31:0] out_1;
  logic[31:0] out_2;
endmodule

//------------------------------------------------------------------------------
// verilator lint_on unusedsignal
