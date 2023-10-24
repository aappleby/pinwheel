`ifndef PINWHEEL_RTL_PINWHEEL_H
`define PINWHEEL_RTL_PINWHEEL_H

`include "metron/metron_tools.sv"

`include "pinwheel/metron/block_ram.sv"
`include "pinwheel/metron/pinwheel_core.sv"
`include "pinwheel/metron/regfile.sv"
`include "pinwheel/metron/test_reg.sv"
`include "pinwheel/metron/tilelink.sv"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal
// verilator lint_off undriven

module pinwheel (
  // global clock
  input logic clock,
  // tock() ports
  input logic tock_reset_in,
  input logic tock__serial_valid,
  input logic[7:0] tock__serial_data,
  // tick() ports
  input logic tick_reset_in,
  input logic tick__serial_valid,
  input logic[7:0] tick__serial_data
);
/*public:*/

  parameter /*const char**/ text_file = "";
  parameter /*const char**/ data_file = "";
  initial begin
  end

  /*metron_noconvert*/ /*pinwheel* clone();*/
  /*metron_noconvert*/ /*size_t size_bytes();*/
  /*metron_noconvert*/ /*bool load_elf(const char* firmware_filename);*/
  /*metron_noconvert*/ /*uint32_t* get_code();*/
  /*metron_noconvert*/ /*uint32_t* get_data();*/
  /*metron_noconvert*/ /*logic<32> get_debug() const;*/

  //----------------------------------------
  // FIXME const local variable should not become parameter

  always_comb begin : tock
    tilelink_d bus_tld;

    bus_tld.d_opcode = 3'bx;
    bus_tld.d_param  = 2'bx;
    bus_tld.d_size   = 3'bx;
    bus_tld.d_source = 1'bx;
    bus_tld.d_sink   = 3'bx;
    bus_tld.d_data   = 32'bx;
    bus_tld.d_error  = 1'bx;
    bus_tld.d_valid  = 1'bx;
    bus_tld.d_ready  = 1'bx;

    if (data_ram_bus_tld.d_valid == 1)  bus_tld = data_ram_bus_tld;
    if (debug_reg_bus_tld.d_valid == 1) bus_tld = debug_reg_bus_tld;

    //----------

    core_tock_reset_in = tock_reset_in;
    core_tock_code_tld = code_ram_bus_tld;
    core_tock_bus_tld = bus_tld;
    core_tock_reg_rdata1 = regs_get_rs1_ret;
    core_tock_reg_rdata2 = regs_get_rs2_ret;
    /*core.tock(reset_in, code_ram.bus_tld, bus_tld, regs.get_rs1(), regs.get_rs2());*/

    debug_reg_tick_tla = core_bus_tla;
    /*debug_reg.tick(core.bus_tla);*/
    code_ram_tick_tla = core_code_tla;
    /*code_ram.tick(core.code_tla);*/
    data_ram_tick_tla = core_bus_tla;
    /*data_ram.tick(core.bus_tla);*/
    core_tick_reset_in = tock_reset_in;
    /*core.tick(reset_in);*/

    regs_tick_in = core_core_to_reg;
    /*regs.tick(core.core_to_reg);*/
  end

  //----------------------------------------
  // FIXME trace modules individually

  always_ff @(posedge clock) begin : tick
  end

  //----------------------------------------

  /* metron_internal */ pinwheel_core core(
    // Global clock
    .clock(clock),
    // Output signals
    .bus_tla(core_bus_tla),
    .code_tla(core_code_tla),
    .core_to_reg(core_core_to_reg),
    // tock() ports
    .tock_reset_in(core_tock_reset_in),
    .tock_code_tld(core_tock_code_tld),
    .tock_bus_tld(core_tock_bus_tld),
    .tock_reg_rdata1(core_tock_reg_rdata1),
    .tock_reg_rdata2(core_tock_reg_rdata2),
    // tick() ports
    .tick_reset_in(core_tick_reset_in)
  );
  logic core_tock_reset_in;
  tilelink_d core_tock_code_tld;
  tilelink_d core_tock_bus_tld;
  logic[31:0] core_tock_reg_rdata1;
  logic[31:0] core_tock_reg_rdata2;
  logic core_tick_reset_in;
  tilelink_a core_bus_tla;
  tilelink_a core_code_tla;
  regfile_in core_core_to_reg;
  /* metron_internal */ regfile       regs(
    // Global clock
    .clock(clock),
    // tick() ports
    .tick_in(regs_tick_in),
    // get_rs1() ports
    .get_rs1_ret(regs_get_rs1_ret),
    // get_rs2() ports
    .get_rs2_ret(regs_get_rs2_ret)
  );
  regfile_in regs_tick_in;
  logic[31:0] regs_get_rs1_ret;
  logic[31:0] regs_get_rs2_ret;
  /* metron_internal */ block_ram #(
    // Template Parameters
    .addr_mask(32'hF0000000),
    .addr_tag(32'h00000000),
    // Constructor Parameters
    .filename(text_file)
  ) code_ram(
    // Global clock
    .clock(clock),
    // Output signals
    .bus_tld(code_ram_bus_tld),
    // tick() ports
    .tick_tla(code_ram_tick_tla)
  );
  tilelink_a code_ram_tick_tla;
  tilelink_d code_ram_bus_tld;
  /* metron_internal */ block_ram #(
    // Template Parameters
    .addr_mask(32'hF0000000),
    .addr_tag(32'h80000000),
    // Constructor Parameters
    .filename(data_file)
  ) data_ram(
    // Global clock
    .clock(clock),
    // Output signals
    .bus_tld(data_ram_bus_tld),
    // tick() ports
    .tick_tla(data_ram_tick_tla)
  );
  tilelink_a data_ram_tick_tla;
  tilelink_d data_ram_bus_tld; // FIXME having this named data and a field inside block_ram named data breaks context resolve
  /* metron_internal */ test_reg  #(
    // Template Parameters
    .addr_mask(32'hF0000000),
    .addr_tag(32'hF0000000)
  ) debug_reg(
    // Global clock
    .clock(clock),
    // Output registers
    .bus_tld(debug_reg_bus_tld),
    // tick() ports
    .tick_tla(debug_reg_tick_tla)
  );
  tilelink_a debug_reg_tick_tla;
  tilelink_d debug_reg_bus_tld;
endmodule

// verilator lint_on unusedsignal
// verilator lint_off undriven
//------------------------------------------------------------------------------

`endif // PINWHEEL_RTL_PINWHEEL_H
