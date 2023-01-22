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
/*public:*/
  /*
  logic<3>  a_opcode;
  logic<3>  a_param;
  logic<3>  a_size;
  logic<1>  a_source;
  logic<32> a_address;
  logic<4>  a_mask;
  logic<32> a_data;
  logic<1>  a_valid;
  logic<1>  a_ready;
  */


  /*
  logic<3>  d_opcode;
  logic<2>  d_param;
  logic<3>  d_size;
  logic<1>  d_source;
  logic<3>  d_sink;
  logic<32> d_data;
  logic<1>  d_error;
  logic<1>  d_valid;
  logic<1>  d_ready;
  */


  always_comb begin : tock
  end

/*private:*/

  always_ff @(posedge clock) begin : tick
    tld.d_size <= tla.a_size;

    if (tla.a_opcode == TL::Get) begin
      tld.d_opcode <= TL::AccessAckData;
      tld.d_param  <= 0;
      tld.d_size   <= tla.a_size;
      tld.d_source <= tla.a_source;
      tld.d_sink   <= 3'bx;
      tld.d_data   <= 32'hDEADBEEF;
      tld.d_error  <= 0;
      tld.d_valid  <= 1;
      tld.d_ready  <= 1;
    end
    else if (tla.a_opcode == TL::PutFullData) begin
      tld.d_opcode <= TL::AccessAck;
      tld.d_param  <= 0;
      tld.d_size   <= tla.a_size;
      tld.d_source <= tla.a_source;
      tld.d_sink   <= 0;
      tld.d_data   <= 32'bx;
      tld.d_error  <= 0;
      tld.d_valid  <= 0;
      tld.d_ready  <= 1;
    end
    else if (tla.a_opcode == TL::PutPartialData) begin
      tld.d_opcode <= TL::AccessAck;
      tld.d_param  <= 0;
      tld.d_size   <= tla.a_size;
      tld.d_source <= tla.a_source;
      tld.d_sink   <= 0;
      tld.d_data   <= 32'bx;
      tld.d_error  <= 0;
      tld.d_valid  <= 0;
      tld.d_ready  <= 1;
    end
  end
endmodule

// verilator lint_on unusedsignal
// verilator lint_on undriven
