`include "metron_tools.sv"

//------------------------------------------------------------------------------

module block_ram (
  // global clock
  input logic clock,
  // tock() ports
  input logic[11:0] tock_addr_,
  input logic[31:0] tock_wdata_,
  input logic[3:0] tock_wmask_,
  input logic tock_wren_,
  // rdata() ports
  output logic[31:0] rdata_ret
);
/*public:*/

  always_comb begin : tock
    addr  = tock_addr_;
    wdata = tock_wdata_;
    wmask = tock_wmask_;
    wren  = tock_wren_;
  end

  always_comb begin : rdata
    rdata_ret = data_out;
  end

  always_ff @(posedge clock) begin : tick
    if (wren) begin
      logic[31:0] old_data;
      logic[31:0] new_data;
      old_data = data[addr[11:2]];
      new_data = wdata;
      if (addr[0]) new_data = new_data << 8;
      if (addr[1]) new_data = new_data << 16;
      new_data = ((wmask[0] ? new_data : old_data) & 32'h000000FF) |
                 ((wmask[1] ? new_data : old_data) & 32'h0000FF00) |
                 ((wmask[2] ? new_data : old_data) & 32'h00FF0000) |
                 ((wmask[3] ? new_data : old_data) & 32'hFF000000);

      data[addr[11:2]] <= new_data;
      data_out <= new_data;
    end
    else begin
      data_out <= data[addr[11:2]];
    end
  end

  // metron_noconvert
  /*uint32_t* get_data() { return (uint32_t*)data; }*/
  // metron_noconvert
  /*const uint32_t* get_data() const { return (uint32_t*)data; }*/

  // metron_internal
  logic[11:0] addr;
  logic[31:0] wdata;
  logic[3:0]  wmask;
  logic  wren;

  logic[31:0] data[16384];
  logic[31:0] data_out;
endmodule

//------------------------------------------------------------------------------
