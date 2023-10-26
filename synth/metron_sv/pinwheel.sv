`include "metron_tools.sv"

`include "block_ram.sv"
`include "pinwheel_core.sv"
`include "regfile.sv"
`include "serial.sv"
`include "test_reg.sv"
`include "tilelink.sv"

// metron_noconvert
/*#include "console.h"
*/
`ifndef PINWHEEL_H
`define PINWHEEL_H

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
/*public*/

  parameter text_file = "";
  parameter data_file = "";
  initial
  begin
  end

  // metron_noconvert
  /*pinwheel* clone() {
    pinwheel* p = new pinwheel();
    memcpy(p, this, sizeof(*this));
    return p;
  }*/

  // metron_noconvert
  /*size_t size_bytes() { return sizeof(*this); }*/
  // metron_noconvert
  /*bool load_elf(const char* firmware_filename);*/
  // metron_noconvert
  /*uint32_t* get_code() { return code_ram.get_data(); }*/
  // metron_noconvert
  /*uint32_t* get_data() { return data_ram.get_data(); }*/
  // metron_noconvert
  /*logic<32> get_debug() const { return debug_reg.get(); }*/

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

    if (data_ram.bus_tld.d_valid == 1)  bus_tld = data_ram_bus_tld;
    if (debug_reg.bus_tld.d_valid == 1) bus_tld = debug_reg_bus_tld;

    //----------
    core_tock_reset_in = tock_reset_in;
    core_tock_code_tld = code_ram_bus_tld;
    core_tock_bus_tld = bus_tld;
    core_tock_reg_rdata1 = regs_get_rs1_ret;
    core_tock_reg_rdata2 = regs_get_rs2_ret;

    debug_reg_tick_tla = core_bus_tla;

    code_ram_tick_tla = core_code_tla;
    data_ram_tick_tla = core_bus_tla;
    core_tick_reset_in = tock_reset_in;
    regs_tick_in = core_core_to_reg;


    // metron_noconvert
    /*console1.tick(reset_in, core.bus_tla);*/
    // metron_noconvert
    /*console2.tick(reset_in, core.bus_tla);*/
    // metron_noconvert
    /*console3.tick(reset_in, core.bus_tla);*/
    // metron_noconvert
    /*console4.tick(reset_in, core.bus_tla);*/
  end

  //----------------------------------------
  // FIXME trace modules individually

  always_ff @(posedge clock) begin : tick
  end

  //----------------------------------------
  // metron_internal

  pinwheel_core core(
    // Global clock
    .clock(clock),
    // Output signals
    .bus_tla(core_bus_tla),
    .code_tla(core_code_tla),
    .core_to_reg(core_core_to_reg),
    .sig_code_addr(core_sig_code_addr),
    .sig_code_wdata(core_sig_code_wdata),
    .sig_code_wmask(core_sig_code_wmask),
    .sig_code_wren(core_sig_code_wren),
    .sig_bus_addr(core_sig_bus_addr),
    .sig_bus_rden(core_sig_bus_rden),
    .sig_bus_wdata(core_sig_bus_wdata),
    .sig_bus_wmask(core_sig_bus_wmask),
    .sig_bus_wren(core_sig_bus_wren),
    .sig_rf_raddr1(core_sig_rf_raddr1),
    .sig_rf_raddr2(core_sig_rf_raddr2),
    .sig_rf_waddr(core_sig_rf_waddr),
    .sig_rf_wdata(core_sig_rf_wdata),
    .sig_rf_wren(core_sig_rf_wren),
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
  logic[31:0] core_sig_code_addr;
  logic[31:0] core_sig_code_wdata;
  logic[3:0]  core_sig_code_wmask;
  logic  core_sig_code_wren;
  logic[31:0] core_sig_bus_addr;
  logic  core_sig_bus_rden;
  logic[31:0] core_sig_bus_wdata;
  logic[3:0]  core_sig_bus_wmask;
  logic  core_sig_bus_wren;
  logic[7:0]  core_sig_rf_raddr1;
  logic[7:0]  core_sig_rf_raddr2;
  logic[7:0]  core_sig_rf_waddr;
  logic[31:0] core_sig_rf_wdata;
  logic  core_sig_rf_wren;

  regfile       regs(
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



  block_ram #(
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


  // metron_noconvert
  /*Console  <0xF0000000, 0x40000000> console1;*/
  // metron_noconvert
  /*Console  <0xF0000000, 0x50000000> console2;*/
  // metron_noconvert
  /*Console  <0xF0000000, 0x60000000> console3;*/
  // metron_noconvert
  /*Console  <0xF0000000, 0x70000000> console4;*/
  block_ram #(
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
  tilelink_d data_ram_bus_tld;
 // FIXME having this named data and a field inside block_ram named data breaks context resolve
  test_reg #(
    // Template Parameters
    .addr_mask(32'hF0000000),
    .addr_tag(32'hF0000000)
    // Constructor Parameters
  ) debug_reg(
    // Global clock
    .clock(clock),
    // Output registers
    .bus_tld(debug_reg_bus_tld),
    // tick() ports
    .tick_tla(debug_reg_tick_tla)
  );
  logic[3:0] debug_reg_expand_bitmask_mask;
  tilelink_a debug_reg_tick_tla;
  tilelink_d debug_reg_bus_tld;
  logic[31:0] debug_reg_expand_bitmask_ret;

endmodule

// verilator lint_on unusedsignal
// verilator lint_off undriven
//------------------------------------------------------------------------------

`endif
