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

  localparam ram_width = 8;
  localparam ram_depth = 1024;

  localparam ram_addr_bits = $clog2(ram_depth);
  localparam ram_word_bits = ram_width;

  logic[ram_addr_bits-1:0] raddr;
  logic[ram_word_bits-1:0] rdata;

  logic[ram_addr_bits-1:0] waddr;
  logic[ram_word_bits-1:0] wdata;
  logic wren;

  block_ram #(.filename("data/message.hex"), .width(ram_width), .depth(ram_depth))
  my_ram(
    .rclk (CLK),
    .raddr(raddr),
    .rdata(rdata),
    .wclk (CLK),
    .waddr(waddr),
    .wdata(wdata),
    .wren (wren),
  );

  //----------------------------------------

  localparam thread_count = 4;

  localparam reg_count = 32;
  localparam reg_width = 32;
  localparam reg_total = reg_count * thread_count;
  localparam reg_addr_bits = $clog2(reg_count * thread_count);
  localparam reg_word_bits = 32;

  logic[reg_addr_bits-1:0] reg_raddr0;
  logic[reg_word_bits-1:0] reg_rdata0;
  logic[reg_addr_bits-1:0] reg_raddr1;
  logic[reg_word_bits-1:0] reg_rdata1;
  logic[reg_addr_bits-1:0] reg_waddr;
  logic[reg_word_bits-1:0] reg_wdata;
  logic reg_wren;

  pinwheel_regfile #(.reg_count(reg_count), .reg_width(reg_width), .thread_count(thread_count))
  regfile(
    .clk   (CLK),
    .raddr0(reg_raddr0),
    .rdata0(reg_rdata0),
    .raddr1(reg_raddr1),
    .rdata1(reg_rdata1),
    .waddr (reg_waddr),
    .wdata (reg_wdata),
    .wren  (reg_wren)
  );

  //----------------------------------------

  localparam code_size_bytes = 4096;
  localparam code_addr_bits = $clog2(code_size_bytes / 4);
  localparam code_word_bits = 32;

  logic[code_addr_bits-1:0] code_raddr;
  logic[code_word_bits-1:0] code_rdata;
  logic[code_addr_bits-1:0] code_waddr;
  logic[code_word_bits-1:0] code_wdata;
  logic code_wren;

  pinwheel_ram #(.size_bytes(code_size_bytes))
  code_ram(
    .clk  (CLK),
    .raddr(code_raddr),
    .rdata(code_rdata),
    .waddr(code_waddr),
    .wdata(code_wdata),
    .wren (code_wren),
  );

  //----------------------------------------

  localparam data_size_bytes = 4096;
  localparam data_addr_bits = $clog2(data_size_bytes / 4);
  localparam data_word_bits = 32;

  logic[data_addr_bits-1:0] data_raddr;
  logic[data_word_bits-1:0] data_rdata;
  logic[data_addr_bits-1:0] data_waddr;
  logic[data_word_bits-1:0] data_wdata;
  logic data_wren;

  pinwheel_ram data_ram
  (
    .clk  (CLK),
    .raddr(data_raddr),
    .rdata(data_rdata),
    .waddr(data_waddr),
    .wdata(data_wdata),
    .wren (data_wren),
  );

  //----------------------------------------

  logic[63:0] counter;
  logic[ram_addr_bits-1:0] index;

  always_comb begin
    raddr = index;
  end

  always @(posedge CLK) begin
    if (reset) begin
      counter <= 0;
      index <= 0;
    end
    else begin
      if (counter == 'h3FFFF) begin
        counter <= 0;
        if (index == ram_depth - 1) begin
          index <= 0;
        end else begin
          index <= index + 1;
        end
      end
      else begin
        counter <= counter + 1;
      end
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
