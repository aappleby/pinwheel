`include "metron_tools.sv"
`include "tilelink.sv"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal

module block_ram (
  // global clock
  input logic clock,
  // rdata() ports
  output logic[31:0] rdata_ret,
  // get_tld() ports
  output tilelink_d get_tld_ret,
  // tick() ports
  input tilelink_a tick_tla
);
/*public:*/

  parameter filename = "";
  initial begin
    if (filename) $readmemh(filename, data);
    oe = 0;
  end

  always_comb begin : rdata
    rdata_ret = data_out;
  end

  always_comb begin : get_tld
    tilelink_d bus_tld;
    bus_tld.d_opcode = 0;
    bus_tld.d_param = 0;
    bus_tld.d_size = 0;
    bus_tld.d_source = 0;
    bus_tld.d_sink = 0;
    bus_tld.d_data = 0;
    bus_tld.d_error = 0;
    bus_tld.d_valid = 0;
    bus_tld.d_ready = 0;
    get_tld_ret = bus_tld;
  end

  always_ff @(posedge clock) begin : tick
    if (tick_tla.a_valid) begin
      if (tick_tla.a_opcode == TL::PutPartialData) begin
        logic[31:0] old_data;
        logic[31:0] new_data;
        old_data = data[tick_tla.a_address[11:2]];
        new_data = tick_tla.a_data;
        if (tick_tla.a_address[0]) new_data = new_data << 8;
        if (tick_tla.a_address[1]) new_data = new_data << 16;
        new_data = ((tick_tla.a_mask[0] ? new_data : old_data) & 32'h000000FF) |
                  ((tick_tla.a_mask[1] ? new_data : old_data) & 32'h0000FF00) |
                  ((tick_tla.a_mask[2] ? new_data : old_data) & 32'h00FF0000) |
                  ((tick_tla.a_mask[3] ? new_data : old_data) & 32'hFF000000);

        data[tick_tla.a_address[11:2]] <= new_data;
        data_out <= new_data;
        oe <= 1;
      end
      else begin
        data_out <= data[tick_tla.a_address[11:2]];
        oe <= 1;
      end
    end
    else begin
      data_out <= 32'bx;
      oe <= 0;
    end
  end

  // metron_noconvert
  /*uint32_t* get_data() { return (uint32_t*)data; }*/
  // metron_noconvert
  /*const uint32_t* get_data() const { return (uint32_t*)data; }*/

  // metron_internal
  logic[31:0] data[16384];
  logic[31:0] data_out;
  logic  oe;
endmodule

// verilator lint_on unusedsignal
//------------------------------------------------------------------------------
