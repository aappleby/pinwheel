// Serial device with a TL-UL interface

`include "metron_tools.sv"
`include "tilelink.sv"

// verilator lint_off unusedsignal
// verilator lint_off undriven

module Serial (
  // global clock
  input logic clock,
  // input signals
  input tilelink_a tla,
  // output registers
  output tilelink_d tld
);
/*public*/

  always_comb begin : tock
  end

/*private*/

  logic[31:0] test_reg;

  always_ff @(posedge clock) begin : tick
    tld.d_param  = 0;
    tld.d_size   = tla.a_size;
    tld.d_source = tla.a_source;
    tld.d_sink   = 3'bx;
    tld.d_data   = 32'bx;
    tld.d_error  = 0;
    tld.d_valid  = 0;
    tld.d_ready  = 1;

    if (tla.a_opcode == TL::Get) begin
      tld.d_opcode = TL::AccessAckData;
      if (tla.a_address[31:28] == 4'h5) begin
        tld.d_data   = test_reg;
        tld.d_valid  = 1;
      end
    end
    else if (tla.a_opcode == TL::PutFullData || tla.a_opcode == TL::PutPartialData) begin
      logic[31:0] bitmask;
      tld.d_opcode = TL::AccessAck;
      bitmask = expand_bitmask(tla.a_mask);
      test_reg <= (test_reg & ~bitmask) | (tla.a_data & bitmask);
    end
  end

  function logic[31:0] expand_bitmask(logic[3:0] mask);
    expand_bitmask = {{8 {mask[3]}}, {8 {mask[2]}}, {8 {mask[1]}}, {8 {mask[0]}}};
  endfunction

endmodule

// verilator lint_on unusedsignal
// verilator lint_on undriven
