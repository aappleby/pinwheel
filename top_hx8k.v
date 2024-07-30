`default_nettype none

`include "SB_PLL40_CORE.v"

module top(
  input  logic EXT_CLK,
  input  logic SER_RX,
  output logic SER_TX,
  output logic [7:0] LEDS,
  output logic PROBE1,
  output logic PROBE2,
  output logic [15:0] DSLOGIC,
  output logic [7:0] SALEAE,
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

  // double f_pfd = f_pllin / (divr + 1);
  // int filter_range = f_pfd < 17 ? 1 : f_pfd < 26 ? 2 : f_pfd < 44 ? 3 : f_pfd < 66 ? 4 : f_pfd < 101 ? 5 : 6;
  // assert (f_pfd >= 10 && f_pfd <= 133);
  // double f_vco = f_pfd * (divf + 1);
  // assert(f_vco >= 533 && f_vco <= 1066);
  // double fout = f_vco * exp2(-divq);

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

  logic clock = clock_8M;
  localparam clock_rate = 8000000;

  //----------------------------------------------------------------------------

  localparam baud_rate = 1000000;

  localparam clocks_per_bit  = clock_rate / baud_rate;
  localparam bits_per_byte = 10; // 1 start + 8 data + 1 stop

  localparam delay_mid = (clocks_per_bit / 2) - 1;
  localparam delay_max = clocks_per_bit - 1;
  localparam count_max = 18; // 1 start + 8 data + 9 stop to make sure we're really stopped :D

  localparam delay_width = $clog2(delay_max);
  localparam count_width = $clog2(count_max);

  //----------------------------------------------------------------------------
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

  //----------------------------------------------------------------------------

  logic[15:0] message_delay;
  logic[7:0]  message_out;
  logic[7:0]  message_count;
  logic       message_valid;
  logic       message_early;

  always @(posedge clock) begin
    if (reset) begin
      message_delay <= 499;
    end else begin
      if (message_delay) begin
        message_delay <= message_delay - 1;
        message_out   <= 8'b0;
        message_valid <= 0;
      end else begin
        message_delay <= 499;
        message_out   <= message_count;
        message_count <= message_count + 1;
        message_valid <= 1;
      end

      message_early <= message_delay == 5;

    end
  end

  //----------------------------------------------------------------------------
  // Trivial uart string transmitter

  logic[delay_width-1:0] ping_delay;

  logic[67:0] ping = {
    // message interval
    8'b11111111,

    // "Ping!\n" = 0x50 0x69 0x6E 0x67 0x21 0x0A
    // stop bit, byte, start bit

    1'b1, 8'h0A, 1'b0,
    1'b1, 8'h21, 1'b0,
    1'b1, 8'h67, 1'b0,
    1'b1, 8'h6E, 1'b0,
    1'b1, 8'h69, 1'b0,
    1'b1, 8'h50, 1'b0
  };

  always @(posedge clock) begin
    if (reset) begin
        ping_delay <= 0;
    end else begin
      if (ping_delay == clocks_per_bit - 1) begin
        ping <= {ping[0], ping[67:1]};
        ping_delay <= 0;
      end else begin
        ping_delay <= ping_delay + 1;
      end
    end
  end

  //----------------------------------------------------------------------------

  // 0    1    2    3    4    5    6    7    8    9    10   11   12   13   14
  // 0123 0123 0123 0123 0123 0123 0123 0123 0123 0123 0123 0123 0123 0123 0123
  //   S    B    B    B    B    B    B    B    B    E
  //                                                V

  logic rx_in;

  logic[bits_per_byte-1:0]   rx_shift;
  logic[delay_width-1:0] rx_delay;
  logic[count_width-1:0] rx_count;

  logic[7:0] rx_data;
  logic      rx_valid;

  always @(posedge clock) begin
    if (reset) begin
      rx_shift <= 0;
      rx_valid <= 0;
      rx_delay <= clocks_per_bit;
      rx_count <= bits_per_byte;
    end else begin
      if (rx_count == bits_per_byte) begin
        if (rx_in == 0) begin
          rx_delay <= 0;
          rx_count <= 0;
        end
      end

      if (rx_delay == clocks_per_bit/2) begin
        rx_shift <= {rx_in, rx_shift[bits_per_byte-1:1]};
      end

      if (rx_delay == clocks_per_bit - 1) begin
        if (rx_count < bits_per_byte) begin
          rx_count <= rx_count + 1;
        end
      end

      if (rx_count == count_max && rx_delay == 0) begin
        rx_data  <= rx_shift[7:0];
        rx_valid <= 1;
      end else begin
        rx_valid <= 0;
      end

    end
  end


  assign rx_in = ping[0];

  //----------------------------------------------------------------------------

`ifdef 0
  logic[delay_width-1:0] tx_bit_delay;
  logic[count_width-1:0] tx_bit_count;
  logic[9:0]                 tx_shift;
  logic                      tx_ready;

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
`endif

  //----------------------------------------------------------------------------

  //assign DSLOGIC[0] = ping[0];

  //assign PROBE1 = clock_125K;
  //assign PROBE2 = clock_8M;

  //assign DSLOGIC[0]   = message_early;
  //assign DSLOGIC[7:1] = message_out[6:0];

  always @(posedge pll_clock) begin
    if (reset) begin
      DSLOGIC <= 16'b0000000000000001;
    end else begin
      DSLOGIC <= {DSLOGIC[14:0], DSLOGIC[15]};
    end
  end

  always @(posedge clock) begin
    if (reset) begin
      SALEAE <= 8'b00000001;
    end else begin
      SALEAE <= {SALEAE[0], SALEAE[7:1]};
    end
  end

endmodule
