`include "metron_tools.sv"

`include "block_ram.sv"
// metron_noconvert
/*#include "console.h"*/
`include "constants.sv"
`include "pinwheel_core.sv"

// Address Map
// 0x0xxxxxxx - Code
// 0x8xxxxxxx - Data
// 0xExxxxxxx - Regfiles
// 0xFxxxxxxx - Debug registers

//------------------------------------------------------------------------------

module pinwheel (
  // global clock
  input logic clock,
  // tock() ports
  input logic tock_reset_in,
  // tick() ports
  input logic tick_reset_in
);
/*public:*/

  parameter text_file = "";
  parameter data_file = "";
  initial begin
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
  /*logic<32> get_debug() const { return debug_reg; }*/

  //----------------------------------------
  // FIXME const local variable should not become parameter

  always_comb begin : tock
    logic[31:0] code_to_core;
    logic[31:0] bus_to_core;
    logic[3:0] bus_tag_b;
    logic debug_cs_b;

    code_to_core = code_ram_rdata_ret;
    bus_to_core  = data_ram_rdata_ret;

    if (debug_reg_cs) bus_to_core = debug_reg;

    /*reset_in,*/
    core_tock_code_rdata = code_to_core;
    core_tock_bus_rdata = bus_to_core;

    bus_tag_b = core_sig_bus_addr[31:28];
    debug_cs_b = bus_tag_b == 4'hF;
    debug_reg_next = debug_reg;
    debug_reg_cs_next = debug_cs_b;
    if (core_sig_bus_wren && debug_cs_b) debug_reg_next = core_sig_bus_wdata;
    code_ram_tick_addr = 12'(core_sig_code_addr);
    code_ram_tick_cs = 1;
    code_ram_tick_wdata = core_sig_code_wdata;
    code_ram_tick_wmask = core_sig_code_wmask;
    code_ram_tick_wren = core_sig_code_wren && bus_tag_b == 4'h0;

    data_ram_tick_addr = 12'(core_sig_bus_addr);
    data_ram_tick_cs = bus_tag_b == 4'h8;
    data_ram_tick_wdata = core_sig_bus_wdata;
    data_ram_tick_wmask = core_sig_bus_wmask;
    data_ram_tick_wren = core_sig_bus_wren  && bus_tag_b == 4'h8;

    // metron_noconvert
    /*console1.tick(reset_in, bus_tag_b == 0x4 && core.sig_bus_wren, core.sig_bus_wdata);*/
    // metron_noconvert
    /*console2.tick(reset_in, bus_tag_b == 0x5 && core.sig_bus_wren, core.sig_bus_wdata);*/
    // metron_noconvert
    /*console3.tick(reset_in, bus_tag_b == 0x6 && core.sig_bus_wren, core.sig_bus_wdata);*/
    // metron_noconvert
    /*console4.tick(reset_in, bus_tag_b == 0x7 && core.sig_bus_wren, core.sig_bus_wdata);*/
    core_tick_reset_in = tock_reset_in;

  end

  //----------------------------------------
  // FIXME trace modules individually

  always_ff @(posedge clock) begin : tick
    if (tick_reset_in) begin
      debug_reg <= 0;
      debug_reg_cs <= 0;
    end
    else begin
      debug_reg <= debug_reg_next;
      debug_reg_cs <= debug_reg_cs_next;
    end
  end

  //----------------------------------------

  /* verilator lint_off UNUSED */

  // metron_internal
  pinwheel_core core(
    // Global clock
    .clock(clock),
    // Output signals
    .sig_code_addr(core_sig_code_addr),
    .sig_code_wdata(core_sig_code_wdata),
    .sig_code_wmask(core_sig_code_wmask),
    .sig_code_wren(core_sig_code_wren),
    .sig_bus_addr(core_sig_bus_addr),
    .sig_bus_wdata(core_sig_bus_wdata),
    .sig_bus_wmask(core_sig_bus_wmask),
    .sig_bus_wren(core_sig_bus_wren),
    // tock() ports
    .tock_code_rdata(core_tock_code_rdata),
    .tock_bus_rdata(core_tock_bus_rdata),
    // tick() ports
    .tick_reset_in(core_tick_reset_in)
  );
  logic[31:0] core_tock_code_rdata;
  logic[31:0] core_tock_bus_rdata;
  logic core_tick_reset_in;
  logic[31:0] core_sig_code_addr;
  logic[31:0] core_sig_code_wdata;
  logic[3:0]  core_sig_code_wmask;
  logic  core_sig_code_wren;
  logic[31:0] core_sig_bus_addr;
  logic[31:0] core_sig_bus_wdata;
  logic[3:0]  core_sig_bus_wmask;
  logic  core_sig_bus_wren;


  logic[31:0] debug_reg_next;
  logic[31:0] debug_reg;
  logic  debug_reg_cs_next;
  logic  debug_reg_cs;

  block_ram #(
    // Constructor Parameters
    .filename(text_file)
  ) code_ram(
    // Global clock
    .clock(clock),
    // rdata() ports
    .rdata_ret(code_ram_rdata_ret),
    // tick() ports
    .tick_addr(code_ram_tick_addr),
    .tick_cs(code_ram_tick_cs),
    .tick_wdata(code_ram_tick_wdata),
    .tick_wmask(code_ram_tick_wmask),
    .tick_wren(code_ram_tick_wren)
  );
  logic[11:0] code_ram_tick_addr;
  logic code_ram_tick_cs;
  logic[31:0] code_ram_tick_wdata;
  logic[3:0] code_ram_tick_wmask;
  logic code_ram_tick_wren;
  logic[31:0] code_ram_rdata_ret;

  block_ram #(
    // Constructor Parameters
    .filename(data_file)
  ) data_ram(
    // Global clock
    .clock(clock),
    // rdata() ports
    .rdata_ret(data_ram_rdata_ret),
    // tick() ports
    .tick_addr(data_ram_tick_addr),
    .tick_cs(data_ram_tick_cs),
    .tick_wdata(data_ram_tick_wdata),
    .tick_wmask(data_ram_tick_wmask),
    .tick_wren(data_ram_tick_wren)
  );
  logic[11:0] data_ram_tick_addr;
  logic data_ram_tick_cs;
  logic[31:0] data_ram_tick_wdata;
  logic[3:0] data_ram_tick_wmask;
  logic data_ram_tick_wren;
  logic[31:0] data_ram_rdata_ret;
 // FIXME having this named data and a field inside block_ram named data breaks context resolve

  /*logic<32> gpio_dir;*/
  /*logic<32> gpio_in;*/
  /*logic<32> gpio_out;*/

  // metron_noconvert
  /*Console console1;*/
  // metron_noconvert
  /*Console console2;*/
  // metron_noconvert
  /*Console console3;*/
  // metron_noconvert
  /*Console console4;*/

  /* verilator lint_on UNUSED */
endmodule

// verilator lint_on unusedsignal

//------------------------------------------------------------------------------
