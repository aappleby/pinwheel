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

// verilator lint_off unusedsignal

//------------------------------------------------------------------------------

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

    code_to_core = code_ram_rdata_ret;
    bus_to_core  = data_ram_rdata_ret;

    if (debug_reg_cs) bus_to_core = debug_reg;
    if (serial_cs)    bus_to_core = serial_reg;

    //----------
    core_tock_reset_in = tock_reset_in;
    core_tock_code_rdata = code_to_core;
    core_tock_bus_rdata = bus_to_core;
    core_tock_reg_rdata1 = regs_get_rs1_ret;
    core_tock_reg_rdata2 = regs_get_rs2_ret;

    bus_tag_b = core_sig_bus_addr[31:28];

    //----------
    regs_tick_raddr1 = core_sig_rf_raddr1;
    regs_tick_raddr2 = core_sig_rf_raddr2;
    regs_tick_waddr = core_sig_rf_waddr;
    regs_tick_wdata = core_sig_rf_wdata;
    regs_tick_wren = core_sig_rf_wren;


    begin
      logic debug_cs_b;
      debug_cs_b = bus_tag_b == 4'hF;
      debug_reg_next = debug_reg;
      debug_reg_cs_next = debug_cs_b;
      if (core_sig_bus_wren && debug_cs_b) debug_reg_next = core_sig_bus_wdata;
    end

    begin
      serial_cs_next = 0;
      serial_valid_next = 0;
      serial_reg_next = 0;
      serial_out_next = 0;
      serial_out_valid_next = 0;
    end

    /*
    {
      logic<1> serial_cs_b = bus_tag_b == 0xC;

      if (core.sig_bus_wren && serial_cs_b) {
        serial_out_next = core.sig_bus_wdata;
        serial_out_valid_next = 1;
      } else {
        serial_out_next = 0;
        serial_out_valid_next = 0;
      }

      if (core.sig_bus_rden && serial_cs_b) {
        serial_cs_next = serial_cs_b;
      }
      else {
        serial_cs_next = 0;
      }
    }
    */
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
  end

  //----------------------------------------
  // FIXME trace modules individually

  always_ff @(posedge clock) begin : tick
    if (tick_reset_in) begin
      debug_reg <= 0;
      debug_reg_cs <= 0;
      serial_cs <= 0;
      serial_out <= 0;
      serial_out_valid <= 0;
      serial_reg <= 0;
    end
    else begin
      debug_reg <= debug_reg_next;
      debug_reg_cs <= debug_reg_cs_next;

      serial_cs        <= serial_cs_next;
      serial_valid     <= serial_valid_next;
      serial_reg       <= serial_reg_next;
      serial_out       <= serial_out_next;
      serial_out_valid <= serial_out_valid_next;
    end
  end

  //----------------------------------------
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
    .tock_code_rdata(core_tock_code_rdata),
    .tock_bus_rdata(core_tock_bus_rdata),
    .tock_reg_rdata1(core_tock_reg_rdata1),
    .tock_reg_rdata2(core_tock_reg_rdata2)
  );
  logic core_tock_reset_in;
  logic[31:0] core_tock_code_rdata;
  logic[31:0] core_tock_bus_rdata;
  logic[31:0] core_tock_reg_rdata1;
  logic[31:0] core_tock_reg_rdata2;
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
    .tick_raddr1(regs_tick_raddr1),
    .tick_raddr2(regs_tick_raddr2),
    .tick_waddr(regs_tick_waddr),
    .tick_wdata(regs_tick_wdata),
    .tick_wren(regs_tick_wren),
    // get_rs1() ports
    .get_rs1_ret(regs_get_rs1_ret),
    // get_rs2() ports
    .get_rs2_ret(regs_get_rs2_ret)
  );
  logic[7:0] regs_tick_raddr1;
  logic[7:0] regs_tick_raddr2;
  logic[7:0] regs_tick_waddr;
  logic[31:0] regs_tick_wdata;
  logic regs_tick_wren;
  logic[31:0] regs_get_rs1_ret;
  logic[31:0] regs_get_rs2_ret;


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

  logic  serial_cs_next;
  logic  serial_valid_next;
  logic[31:0] serial_reg_next;
  logic[31:0] serial_out_next;
  logic  serial_out_valid_next;

  logic  serial_cs;
  logic  serial_valid;
  logic[31:0] serial_reg;
  logic[31:0] serial_out;
  logic  serial_out_valid;

  // metron_noconvert
  /*Console console1;*/
  // metron_noconvert
  /*Console console2;*/
  // metron_noconvert
  /*Console console3;*/
  // metron_noconvert
  /*Console console4;*/
endmodule

// verilator lint_on unusedsignal

//------------------------------------------------------------------------------
