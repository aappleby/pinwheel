`include "metron_tools.sv"

`include "constants.sv"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal

module regfile (
  // global clock
  input logic clock,
  // tick() ports
  input logic[7:0] tick_raddr1,
  input logic[7:0] tick_raddr2,
  input logic[7:0] tick_waddr,
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
    for (i = 0; i < 256; i = i + 1) begin
      data1_hi[i] = 0;
      data1_lo[i] = 0;
      data2_hi[i] = 0;
      data2_lo[i] = 0;
    end
  end

  always_ff @(posedge clock) begin : tick
    if (tick_wren) begin
      out_1 <= tick_raddr1 == tick_waddr ? tick_wdata : {data1_hi[tick_raddr1], data1_lo[tick_raddr1]};
      out_2 <= tick_raddr2 == tick_waddr ? tick_wdata : {data2_hi[tick_raddr2], data2_lo[tick_raddr2]};
      data1_hi[tick_waddr] <= tick_wdata[31:16];
      data1_lo[tick_waddr] <= tick_wdata[15:0];
      data2_hi[tick_waddr] <= tick_wdata[31:16];
      data2_lo[tick_waddr] <= tick_wdata[15:0];
    end
    else begin
      out_1 <= {data1_hi[tick_raddr1], data1_lo[tick_raddr1]};
      out_2 <= {data2_hi[tick_raddr2], data2_lo[tick_raddr2]};
    end
  end

  always_comb begin : get_rs1 get_rs1_ret = out_1; end
  always_comb begin : get_rs2 get_rs2_ret = out_2; end

  // metron_noconvert
  /*const uint32_t get(int index) const {
    return cat(data1_hi[index], data1_lo[index]);
  }*/

  // metron_internal
  logic[15:0] data1_hi[256];
  logic[15:0] data1_lo[256];
  logic[15:0] data2_hi[256];
  logic[15:0] data2_lo[256];
  logic[31:0] out_1;
  logic[31:0] out_2;
endmodule

//------------------------------------------------------------------------------
// verilator lint_on unusedsignal
