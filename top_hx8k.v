`default_nettype none

`include "SB_PLL40_CORE.v"

module top(
  input  logic EXT_CLK,
  input  logic SER_RX,
  output logic SER_TX,
  output logic [7:0] LEDS,
  output logic PROBE1,
  output logic PROBE2,
  output logic [7:0] LOGIC
);

  localparam ext_clock_rate = 12000000;

  /**
  * PLL configuration
  *
  * This Verilog module was generated automatically
  * using the icepll tool from the IceStorm project.
  * Use at your own risk.
  *
  * Given input frequency:        12.000 MHz
  * Requested output frequency:   48.000 MHz
  * Achieved output frequency:    48.000 MHz
  */

  logic pll_clock;
  logic pll_lock;
  localparam pll_clock_rate = 48000000;

  SB_PLL40_CORE #(
    .FEEDBACK_PATH("SIMPLE"),
    .DIVR(4'b0000),		// DIVR =  0
    .DIVF(7'b0111111),	// DIVF = 63
    .DIVQ(3'b100),		// DIVQ =  4
    .FILTER_RANGE(3'b001)	// FILTER_RANGE = 1
  ) uut (
    .LOCK(pll_lock),
    .RESETB(1'b1),
    .BYPASS(1'b0),
    .REFERENCECLK(EXT_CLK),
    .PLLOUTCORE(pll_clock)
  );

  //----------------------------------------------------------------------------

  logic reset = !pll_lock;

  /*
  logic[15:0] reset_counter = 0;
  logic reset = reset_counter != 8'hFFFF;

  always @(posedge EXT_CLK) if (reset) reset_counter <= reset_counter + 1;
  */

  //----------------------------------------------------------------------------

  //logic clock = pll_clock;
  //localparam clock_rate = pll_clock_rate;

  logic clock = EXT_CLK;
  localparam clock_rate = ext_clock_rate;

  //localparam clock_rate = pll_clock_rate / 2;
  //logic clock;
  //always @(posedge pll_clock) clock <= ~clock;

  /*
  localparam clock_rate = pll_clock_rate / 6;
  logic clock;
  */

  logic[7:0] clock_div3;
  logic[7:0] clock_div;
  always @(posedge pll_clock) begin
    if (clock_div3 == 2) begin
      clock_div3 <= 0;
      clock_div <= clock_div + 1;
    end else begin
      clock_div3 <= clock_div3 + 1;
    end
  end

  logic clock_8M   = clock_div[0];
  logic clock_4M   = clock_div[1];
  logic clock_2M   = clock_div[2];
  logic clock_1M   = clock_div[3];
  logic clock_500K = clock_div[4];
  logic clock_250K = clock_div[5];
  logic clock_125K = clock_div[6];


  //logic[7:0] div;
  //always @(posedge pll_clock) div <= div + 1;
  //assign PROBE = div[4];
  //assign PROBE = 1;
  //assign PROBE = pll_clock;

  //----------------------------------------------------------------------------

  localparam baud_rate = 1000000;

  localparam clocks_per_bit  = clock_rate / baud_rate;
  localparam bit_delay_width = $clog2(clocks_per_bit - 1);

  localparam bits_per_byte = 10; // 1 start + 8 data + 1 stop
  localparam bit_count_width = $clog2(bits_per_byte);

  /*
  logic[bit_delay_width-1:0] rx_bit_delay;
  logic[bit_count_width-1:0] rx_bit_count;
  logic[9:0] rx_shift;

  always @(posedge clock) begin
    if (reset) begin
      rx_bit_delay  <= clocks_per_bit - 1;
      rx_bit_count  <= bits_per_byte - 1;
    end else begin

      if (rx_bit_delay == (clocks_per_bit / 2)) begin
        rx_shift <= (SER_RX << 9) | (rx_shift >> 1);
      end

      if (rx_bit_delay < clocks_per_bit - 1) begin
        rx_bit_delay <= rx_bit_delay + 1;
      end else if (rx_bit_count < bits_per_byte - 1) begin
        rx_bit_delay <= 0;
        rx_bit_count <= rx_bit_count + 1;
      end else if (SER_RX == 0) begin
        rx_bit_delay <= 1;
        rx_bit_count <= 0;
      end

    end
  end

  assign LEDS[6:0] = rx_shift[7:1];
  assign LEDS[7] = (rx_shift[0] == 0) && (rx_shift[9] == 1) && (rx_bit_delay == clocks_per_bit - 1) && (rx_bit_count == bits_per_byte - 1);

  */

  // xorshift rng
  logic[31:0] rng;
  always @(posedge clock) begin
    if (reset) begin
      rng <= 1;
    end else begin
      logic[31:0] x;
      x = rng;
      x ^= x << 13;
      x ^= x >> 17;
      x ^= x << 5;
      rng <= x;
    end
  end


  logic[7:0]  rx_data;
  logic       rx_valid;
  logic[15:0] rx_counter;

  always @(posedge clock) begin
    if (reset) begin
      rx_data    <= 8'h00;
      rx_valid   <= 0;
      rx_counter <= 16'hFFFF;
    end else begin
      if (rx_counter) begin
        rx_counter <= rx_counter - 1;
        rx_valid   <= 0;
        rx_data    <= rx_data;
      end else begin
        rx_counter <= 16'hFFFF;
        rx_valid   <= 1;
        rx_data    <= rx_data + 157;
      end
    end
  end

  //assign PROBE1 = rng[0];
  assign PROBE1 = clock_125K;
  //assign PROBE1 = EXT_CLK;
  //assign PROBE2 = rng[1];
  assign PROBE2 = clock_8M;

  //assign LOGIC[0] = rx_valid;
  //assign LOGIC[1] = rng[1];
  //assign LOGIC[2] = rng[2];

  assign LOGIC[0] = clock_8M;
  assign LOGIC[1] = clock_250K;
  assign LOGIC[2] = clock_500K;
  assign LOGIC[3] = clock_1M;
  assign LOGIC[4] = clock_2M;
  assign LOGIC[5] = clock_4M;
  assign LOGIC[6] = clock_8M;
  assign LOGIC[7] = EXT_CLK;

  logic[bit_delay_width-1:0] tx_bit_delay;
  logic[bit_count_width-1:0] tx_bit_count;
  logic[9:0] tx_shift;
  logic      tx_ready;

  //logic tx_idle = (tx_bit_count == bits_per_byte - 1) && (tx_bit_delay == clocks_per_bit - 1);

  always @(posedge clock) begin
    if (reset) begin
      tx_shift <= 10'b1100101110;
      tx_bit_delay <= clocks_per_bit - 1;
    end else begin
      if (tx_bit_delay) begin
        tx_bit_delay <= tx_bit_delay - 1;
      end else begin
        tx_shift <= {tx_shift[0], tx_shift[9:1]};
        tx_bit_delay <= clocks_per_bit - 1;
      end
    end
  end

  //assign SER_TX = tx_idle ? 1 : tx_shift[0];

endmodule
