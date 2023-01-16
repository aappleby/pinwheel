`include "metron_tools.sv"

`include "constants.sv"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal

module regfile (
  // global clock
  input logic clock,
  // tick() ports
  input logic[9:0] tick_raddr1,
  input logic[9:0] tick_raddr2,
  input logic[9:0] tick_waddr,
  input logic[31:0] tick_wdata,
  input logic tick_wren,
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

  always_ff @(posedge clock) begin : tick
    out_1 <= data[tick_raddr1];
    out_2 <= data[tick_raddr2];

    if (tick_wren) data[tick_waddr] <= tick_wdata;

    if (tick_wren && tick_raddr1 == tick_waddr) out_1 <= tick_wdata;
    if (tick_wren && tick_raddr2 == tick_waddr) out_2 <= tick_wdata;
  end

  always_comb begin : get_rs1 get_rs1_ret = out_1; end
  always_comb begin : get_rs2 get_rs2_ret = out_2; end

  // metron_noconvert
  /*const uint32_t* get_data() const {
    return (uint32_t*)data;
  }*/

  // metron_internal
  logic[31:0] data[1024];
  logic[31:0] out_1;
  logic[31:0] out_2;
endmodule

//------------------------------------------------------------------------------
// verilator lint_on unusedsignal
