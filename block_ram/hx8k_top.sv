`include "bram.sv"

//------------------------------------------------------------------------------

module top(
  input  logic      EXT_CLK,
  output logic[7:0] LEDS,
  output logic[7:0] DEBUG
);

  //----------------------------------------

  logic clock;
  assign clock = EXT_CLK;

  //----------------------------------------

  logic[13:0] counter;
  logic[7:0]  msec;

  initial begin
    counter = 0;
    msec = 0;
  end

  always_ff @(posedge clock) begin
    logic[31:0] counter_next;

    counter_next = counter + 1;

    if (counter_next == 12000) begin
      msec <= msec + 1;
      counter <= 0;
    end else begin
      counter <= counter_next;
    end
  end

  //----------------------------------------

  /*
  logic[7:0]  raddr1;
  logic[31:0] rdata1;
  logic[7:0]  raddr2;
  logic[31:0] rdata2;
  logic[7:0]  waddr;
  logic[15:0] wdata;
  logic       wren;

  picosoc_regs2 regs(
    .clock(clock),
    .raddr1(raddr1),
    .rdata1(rdata1),
    .raddr2(raddr2),
    .rdata2(rdata2),
    .waddr(waddr),
    .wdata(wdata),
    .wren(wren),
  );
  */

  logic[7:0]  raddr;
  logic[31:0] rdata;
  logic[7:0]  waddr;
  logic[31:0] wdata;
  logic[3:0]  wmask;
  logic       wren;

  bram_align2_1024_mask ram(
    .clock(clock),
    .raddr(raddr),
    .rdata(rdata),
    .waddr(waddr),
    .wdata(wdata),
    .wmask(wmask),
    .wren(wren),
  );

  //----------------------------------------

  always_comb begin

    raddr = 0;
    waddr = 0;
    wdata = 0;
    wmask = 0;
    wren  = 0;

    wmask = 4'b1111;

    case (msec[2:0])
    0: begin waddr = 0; wdata = 'h01020304; wren = 1; raddr = 0; end
    1: begin waddr = 0; wdata = 'h01020304; wren = 1; raddr = 0; end
    2: begin waddr = 0; wdata = 'h01020304; wren = 1; raddr = 0; end
    3: begin waddr = 0; wdata = 'h01020304; wren = 1; raddr = 0; end
    4: begin waddr = 0; wdata = 'h01020304; wren = 1; raddr = 0; end
    5: begin waddr = 0; wdata = 'h01020304; wren = 1; raddr = 0; end
    6: begin waddr = 0; wdata = 'h01020304; wren = 1; raddr = 0; end
    7: begin waddr = 0; wdata = 'h01020304; wren = 1; raddr = 0; end
    endcase

  end

  always_ff @(posedge clock) begin

    case (msec[2:0])
    0: begin LEDS <= rdata[31:24]; DEBUG <= rdata[31:24]; end
    1: begin LEDS <= rdata[23:16]; DEBUG <= rdata[23:16]; end
    2: begin LEDS <= rdata[15: 8]; DEBUG <= rdata[15: 8]; end
    3: begin LEDS <= rdata[ 7: 0]; DEBUG <= rdata[ 7: 0]; end
    4: begin LEDS <= rdata[31:24]; DEBUG <= rdata[31:24]; end
    5: begin LEDS <= rdata[23:16]; DEBUG <= rdata[23:16]; end
    6: begin LEDS <= rdata[15: 8]; DEBUG <= rdata[15: 8]; end
    7: begin LEDS <= rdata[ 7: 0]; DEBUG <= rdata[ 7: 0]; end
    endcase

  end

  //----------------------------------------

endmodule
