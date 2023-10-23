`ifndef PINWHEEL_RTL_TEST_REG_H
`define PINWHEEL_RTL_TEST_REG_H

`include "metron/metron_tools.sv"
`include "pinwheel/metron/tilelink.sv"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off unusedparam

module test_reg (
  // global clock
  input logic clock,
  // output registers
  output tilelink_d bus_tld,
  // tick() ports
  input tilelink_a tick_tla
);
  parameter addr_mask = 32'hF0000000;
  parameter addr_tag = 32'hF0000000;
/*public:*/


  /* metron_noconvert */ /*logic<32> get() const { return bus_tld.d_data; }*/

  parameter /*logic<32>*/ init = 0;
  initial begin
    bus_tld.d_opcode = TL::AccessAckData;
    bus_tld.d_param  = 2'bx;
    bus_tld.d_size   = 3'bx;
    bus_tld.d_source = 1'bx;
    bus_tld.d_sink   = 3'bx;
    bus_tld.d_data   = init;
    bus_tld.d_error  = 0;
    bus_tld.d_valid  = 0;
    bus_tld.d_ready  = 1;
  end

  always_ff @(posedge clock) begin : tick
    logic cs;
    cs = tick_tla.a_valid && ((tick_tla.a_address & addr_mask) == addr_tag);

    bus_tld.d_valid <= cs;

    if (cs && ((tick_tla.a_opcode == TL::PutPartialData) || (tick_tla.a_opcode == TL::PutFullData))) begin
      logic[31:0] mask;
      mask = expand_bitmask(tick_tla.a_mask);
      bus_tld.d_data <= (bus_tld.d_data & ~mask) | (tick_tla.a_data & mask);
    end
  end

/*private:*/

  function logic[31:0] expand_bitmask(logic[3:0] mask);
    expand_bitmask = {{8 {mask[3]}}, {8 {mask[2]}}, {8 {mask[1]}}, {8 {mask[0]}}};
  endfunction
endmodule

// verilator lint_on unusedsignal
// verilator lint_off unusedparam
//------------------------------------------------------------------------------

`endif // PINWHEEL_RTL_TEST_REG_H
