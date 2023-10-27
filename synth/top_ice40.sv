`ifndef TOP_ICE40_SV
`define TOP_ICE40_SV
`default_nettype none

`include "ice40/SB_PLL40_CORE.v"
`include "block_ram.sv"
`include "pinwheel_regfile.sv"
`include "pinwheel_ram.sv"

//==============================================================================

module uart_ice40(
  input  logic EXT_CLK,
  output logic SER_DCDn,
  output logic SER_DSRn,
  input  logic SER_DTRn,
  input  logic SER_RTSn,
  output logic SER_CTSn,
  output logic SER_TX,
  input  logic SER_RX,

  // On-board LEDs
  output logic [7:0] LEDS,

  // Top pin row connection to logic analyser
  output logic [2:0] DEBUG
);

  logic CLK;
  logic pll_lock;
  localparam pll_clock_rate = 24000000;
  localparam ser_clock_rate = 1200;

  /**
   * PLL configuration
   *
   * This Verilog module was generated automatically
   * using the icepll tool from the IceStorm project.
   * Use at your own risk.
   *
   * Given input frequency:        12.000 MHz
   * Requested output frequency:   24.000 MHz
   * Achieved output frequency:    24.000 MHz
   */
  SB_PLL40_CORE #(
    .FEEDBACK_PATH("SIMPLE"),
    .DIVR(4'b0000),         // DIVR =  0
    .DIVF(7'b0111111),      // DIVF = 63
    .DIVQ(3'b101),          // DIVQ =  5
    .FILTER_RANGE(3'b001)   // FILTER_RANGE = 1
  )
  uut (
    .LOCK(pll_lock),
    .RESETB(1'b1),
    .BYPASS(1'b0),
    .REFERENCECLK(EXT_CLK),
    .PLLOUTCORE(CLK)
  );


  logic reset;
  assign reset = !pll_lock;

  //----------------------------------------

  logic[7:0]  raddr;
  logic[15:0] rdata;

  logic[15:0] wdata;
  logic[7:0]  waddr;
  logic       wren;

  block_ram
  #(
    .filename("data/message.hex"),
    .addr_width(8),
    .data_width(16)
  )
  my_ram(
    .rclk(CLK),
    .raddr(raddr),
    .rdata(rdata),
    .wclk(CLK),
    .waddr(waddr),
    .wdata(wdata),
    .wren(wren),
  );

  //----------------------------------------

  logic[7:0]  reg_raddr0;
  logic[31:0] reg_rdata0;
  logic[7:0]  reg_raddr1;
  logic[31:0] reg_rdata1;
  logic[7:0]  reg_waddr;
  logic[31:0] reg_wdata;
  logic       reg_wren;

  pinwheel_regfile regfile(
    .clk(CLK),
    .raddr0(reg_raddr0),
    .rdata0(reg_rdata0),
    .raddr1(reg_raddr1),
    .rdata1(reg_rdata1),
    .waddr (reg_waddr),
    .wdata (reg_wdata),
    .wren  (reg_wren)
  );

  //----------------------------------------

  logic[9:0]  code_raddr;
  logic[31:0] code_rdata;
  logic[9:0]  code_waddr;
  logic[31:0] code_wdata;
  logic       code_wren;

  pinwheel_ram code_ram
  (
    .clk(CLK),
    .raddr(code_raddr),
    .rdata(code_rdata),
    .waddr(code_waddr),
    .wdata(code_wdata),
    .wren (code_wren),
  );

  //----------------------------------------

  logic[9:0]  data_raddr;
  logic[31:0] data_rdata;
  logic[9:0]  data_waddr;
  logic[31:0] data_wdata;
  logic       data_wren;

  pinwheel_ram data_ram
  (
    .clk(CLK),
    .raddr(data_raddr),
    .rdata(data_rdata),
    .waddr(data_waddr),
    .wdata(data_wdata),
    .wren (data_wren),
  );

  //----------------------------------------

  logic[31:0] counter;

  always_comb begin
    raddr = counter[31:24];
  end

  always @(posedge CLK) begin
    if (reset) begin
      counter <= 0;
    end
    else begin
      counter <= counter + 1;
    end
  end

  //----------------------------------------

  assign LEDS[0] = rdata[0];
  assign LEDS[1] = rdata[1];
  assign LEDS[2] = rdata[2];
  assign LEDS[3] = rdata[3];
  assign LEDS[4] = rdata[4];
  assign LEDS[5] = rdata[5];
  assign LEDS[6] = rdata[6];
  assign LEDS[7] = rdata[7];

endmodule

//==============================================================================

`endif // TOP_ICE40_SV
