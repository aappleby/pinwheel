`include "metron_tools.sv"

//------------------------------------------------------------------------------

module block_ram (
  // global clock
  input logic clock,
  // rdata() ports
  output logic[31:0] rdata_ret,
  // tick() ports
  input logic[11:0] tick_addr,
  input logic tick_cs,
  input logic[31:0] tick_wdata,
  input logic[3:0] tick_wmask,
  input logic tick_wren
);
/*public:*/

  always_comb begin : rdata
    rdata_ret = data_out;
  end

  always_ff @(posedge clock) begin : tick
    if (tick_cs && tick_wren) begin
      logic[31:0] old_data;
      logic[31:0] new_data;
      old_data = data[tick_addr[11:2]];
      new_data = tick_wdata;
      if (tick_addr[0]) new_data = new_data << 8;
      if (tick_addr[1]) new_data = new_data << 16;
      new_data = ((tick_wmask[0] ? new_data : old_data) & 32'h000000FF) |
                 ((tick_wmask[1] ? new_data : old_data) & 32'h0000FF00) |
                 ((tick_wmask[2] ? new_data : old_data) & 32'h00FF0000) |
                 ((tick_wmask[3] ? new_data : old_data) & 32'hFF000000);

      data[tick_addr[11:2]] <= new_data;
      data_out <= tick_cs ? new_data : 32'd0;
    end
    else begin
      data_out <= tick_cs ? data[tick_addr[11:2]] : 32'd0;
    end
  end

  // metron_noconvert
  /*uint32_t* get_data() { return (uint32_t*)data; }*/
  // metron_noconvert
  /*const uint32_t* get_data() const { return (uint32_t*)data; }*/

  // metron_internal
  logic[31:0] data[16384];
  logic[31:0] data_out;
endmodule

//------------------------------------------------------------------------------
