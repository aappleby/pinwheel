// iCE40HX8K has 32 RAM4Ks
// = 16kB

// We need 4 of them for the register file
// = 28 left

// Implementation note:
// Replace the following two modules with wrappers for your SRAM cells.

//------------------------------------------------------------------------------

module pinwheel_regs (
  input  logic       clock,
  input  logic[7:0]  raddr1,
  output logic[31:0] rdata1,
  input  logic[7:0]  raddr2,
  output logic[31:0] rdata2,
  input  logic[7:0]  waddr,
  input  logic[31:0] wdata,
  input  logic       wren,
);

/*
  // SB_RAM40_4K is WRITE-BEFORE-READ, if a write and a read to the same cell
  // occur on the same clock cycle the read will see the NEW value.

  SB_RAM40_4K regs1_lo(
    .RDATA(rdata1[15: 0]), .RCLK(clock), .RCLKE(1), .RE(1), .RADDR(raddr1),
    .WCLK(clock), .WCLKE(1), .WE(wren), .WADDR(waddr), .MASK(16'h0), .WDATA(wdata[15: 0]));

  SB_RAM40_4K regs1_hi(
    .RDATA(rdata1[31:16]), .RCLK(clock), .RCLKE(1), .RE(1), .RADDR(raddr1),
    .WCLK(clock), .WCLKE(1), .WE(wren), .WADDR(waddr), .MASK(16'h0), .WDATA(wdata[31:16]));

  SB_RAM40_4K regs2_lo(
    .RDATA(rdata2[15: 0]), .RCLK(clock), .RCLKE(1), .RE(1), .RADDR(raddr2),
    .WCLK(clock), .WCLKE(1), .WE(wren), .WADDR(waddr), .MASK(16'h0), .WDATA(wdata[15: 0]));

  SB_RAM40_4K regs2_hi(
    .RDATA(rdata2[31:16]), .RCLK(clock), .RCLKE(1), .RE(1), .RADDR(raddr2),
    .WCLK(clock), .WCLKE(1), .WE(wren), .WADDR(waddr), .MASK(16'h0), .WDATA(wdata[31:16]));
*/

  always_ff @(posedge clock) begin
    if (wren) regs[waddr] <= wdata;
    rdata1 <= (wren && (raddr1 == waddr)) ? wdata : regs[raddr1];
    rdata2 <= (wren && (raddr2 == waddr)) ? wdata : regs[raddr2];
  end

  logic[31:0] regs [0:255];

endmodule

//------------------------------------------------------------------------------
// This configuration appears to produce a minimal number of cells in Yosys,
// maybe because not always assigning rdata means it can use the RCLKE signal?

module pinwheel_mem
#(
  parameter integer DEPTH = 256
)
(
  input  logic                    clock,
  input  logic[$clog2(DEPTH)-1:0] addr,
  output logic[31:0]              rdata,
  input  logic[31:0]              wdata,
  input  logic[3:0]               wren,
);
  logic[31:0] ram[0:DEPTH-1];

  always @(posedge clock) begin
    if (wren) begin
      if (wren[0] == 0) ram[addr][ 7: 0] <= wdata[ 7: 0];
      if (wren[1] == 0) ram[addr][15: 8] <= wdata[15: 8];
      if (wren[2] == 0) ram[addr][23:16] <= wdata[23:16];
      if (wren[3] == 0) ram[addr][31:24] <= wdata[31:24];
    end else begin
      rdata <= ram[addr];
    end
  end
endmodule

//------------------------------------------------------------------------------
// Byte-granularity 2k ram made out of 4 512x8s

/*
module bram_align1_2048
(
  input  logic       clock,
  input  logic[10:0] raddr,
  input  logic[10:0] waddr,
  input  logic[31:0] wdata,
  input  logic[1:0]  wsize,
  input  logic       wren,
  output logic[31:0] out
);

  always @* begin

    case (raddr[1:0])
    2'b00: begin raddr0 = (raddr >> 2) + 0; raddr1 = (raddr >> 2) + 0; raddr2 = (raddr >> 2) + 0; raddr3 = (raddr >> 2) + 0; end
    2'b01: begin raddr0 = (raddr >> 2) + 1; raddr1 = (raddr >> 2) + 0; raddr2 = (raddr >> 2) + 0; raddr3 = (raddr >> 2) + 0; end
    2'b10: begin raddr0 = (raddr >> 2) + 1; raddr1 = (raddr >> 2) + 1; raddr2 = (raddr >> 2) + 0; raddr3 = (raddr >> 2) + 0; end
    2'b11: begin raddr0 = (raddr >> 2) + 1; raddr1 = (raddr >> 2) + 1; raddr2 = (raddr >> 2) + 1; raddr3 = (raddr >> 2) + 0; end
    endcase

    case (waddr[1:0])
    2'b00: begin waddr0 = (waddr >> 2) + 0; waddr1 = (waddr >> 2) + 0; waddr2 = (waddr >> 2) + 0; waddr3 = (waddr >> 2) + 0; end
    2'b01: begin waddr0 = (waddr >> 2) + 1; waddr1 = (waddr >> 2) + 0; waddr2 = (waddr >> 2) + 0; waddr3 = (waddr >> 2) + 0; end
    2'b10: begin waddr0 = (waddr >> 2) + 1; waddr1 = (waddr >> 2) + 1; waddr2 = (waddr >> 2) + 0; waddr3 = (waddr >> 2) + 0; end
    2'b11: begin waddr0 = (waddr >> 2) + 1; waddr1 = (waddr >> 2) + 1; waddr2 = (waddr >> 2) + 1; waddr3 = (waddr >> 2) + 0; end
    endcase

    wren0 = 0;
    wren1 = 0;
    wren2 = 0;
    wren3 = 0;

    if (wren) begin
      case (wsize)
      // 1-byte writes
      0: begin
        case (waddr[1:0])
        2'b00: begin wren0 = 1; wren1 = 0; wren2 = 0; wren3 = 0; end
        2'b01: begin wren0 = 0; wren1 = 1; wren2 = 0; wren3 = 0; end
        2'b10: begin wren0 = 0; wren1 = 0; wren2 = 1; wren3 = 0; end
        2'b11: begin wren0 = 0; wren1 = 0; wren2 = 0; wren3 = 1; end
        endcase
      end

      // 2-byte writes
      1: begin
        case (waddr[1:0])
        2'b00: begin wren0 = 1; wren1 = 1; wren2 = 0; wren3 = 0; end
        2'b01: begin wren0 = 0; wren1 = 1; wren2 = 1; wren3 = 0; end
        2'b10: begin wren0 = 0; wren1 = 0; wren2 = 1; wren3 = 1; end
        2'b11: begin wren0 = 1; wren1 = 0; wren2 = 0; wren3 = 1; end
        endcase
      end

      // 4-byte writes
      2: begin
        wren0 = 1; wren1 = 1; wren2 = 1; wren3 = 1;
      end

      // 8-byte writes?
      3: begin
        wren0 = 1; wren1 = 1; wren2 = 1; wren3 = 1;
      end
      endcase
    end

    case (waddr[1:0])
    2'b00: begin wdata0 = wdata[7:0]; wdata1 = wdata[15:8]; wdata2 = wdata[23:16]; wdata3 = wdata[31:24]; end
    2'b01: begin wdata1 = wdata[7:0]; wdata2 = wdata[15:8]; wdata3 = wdata[23:16]; wdata0 = wdata[31:24]; end
    2'b10: begin wdata2 = wdata[7:0]; wdata3 = wdata[15:8]; wdata0 = wdata[23:16]; wdata1 = wdata[31:24]; end
    2'b11: begin wdata3 = wdata[7:0]; wdata0 = wdata[15:8]; wdata1 = wdata[23:16]; wdata2 = wdata[31:24]; end
    endcase

    case (raddr[1:0])
    2'b00: begin out[ 7: 0] = out0; out[15: 8] = out1; out[23:16] = out2; out[31:24] = out3; end
    2'b01: begin out[ 7: 0] = out1; out[15: 8] = out2; out[23:16] = out3; out[31:24] = out0; end
    2'b10: begin out[ 7: 0] = out2; out[15: 8] = out3; out[23:16] = out0; out[31:24] = out1; end
    2'b11: begin out[ 7: 0] = out3; out[15: 8] = out0; out[23:16] = out1; out[31:24] = out2; end
    endcase
  end

  logic[8:0] raddr0;
  logic[8:0] waddr0;
  logic[7:0] wdata0;
  logic      wren0;
  logic[7:0] out0;
  bram_512x8 ram0(.clock(clock), .raddr(raddr0), .waddr(waddr0), .wdata(wdata0), .wren(wren0), .out(out0));

  logic[8:0] raddr1;
  logic[8:0] waddr1;
  logic[7:0] wdata1;
  logic      wren1;
  logic[7:0] out1;
  bram_512x8 ram1(.clock(clock), .raddr(raddr1), .waddr(waddr1), .wdata(wdata1), .wren(wren1), .out(out1));

  logic[8:0] raddr2;
  logic[8:0] waddr2;
  logic[7:0] wdata2;
  logic      wren2;
  logic[7:0] out2;
  bram_512x8 ram2(.clock(clock), .raddr(raddr2), .waddr(waddr2), .wdata(wdata2), .wren(wren2), .out(out2));

  logic[8:0] raddr3;
  logic[8:0] waddr3;
  logic[7:0] wdata3;
  logic      wren3;
  logic[7:0] out3;
  bram_512x8 ram3(.clock(clock), .raddr(raddr3), .waddr(waddr3), .wdata(wdata3), .wren(wren3), .out(out3));

endmodule
*/

//------------------------------------------------------------------------------
// Word-granularity 1k ram made out of 2 256x16s

/*
module bram_align2_1024
(
  input  logic       clock,
  input  logic[9:0]  raddr,
  input  logic[9:0]  waddr,
  input  logic[31:0] wdata,
  input  logic[2:0]  wsize,
  input  logic       wren,
  output logic[31:0] out
);

  always @* begin

    case (raddr[1])
    0: begin raddr0 = (raddr >> 2) + 0; raddr1 = (raddr >> 2) + 0; end
    1: begin raddr0 = (raddr >> 2) + 1; raddr1 = (raddr >> 2) + 0; end
    endcase

    case (waddr[1])
    0: begin waddr0 = (waddr >> 2) + 0; waddr1 = (waddr >> 2) + 0; end
    1: begin waddr0 = (waddr >> 2) + 1; waddr1 = (waddr >> 2) + 0; end
    endcase

    case (waddr[1])
    0: begin wdata0 = wdata[15: 0]; wdata1 = wdata[31:16]; end
    1: begin wdata1 = wdata[15: 0]; wdata0 = wdata[31:16]; end
    endcase

    case (raddr[1])
    0: begin out[15: 0] = out0; out[31:16] = out1; end
    1: begin out[15: 0] = out1; out[31:16] = out0; end
    endcase
  end

  logic[7:0]  raddr0;
  logic[7:0]  waddr0;
  logic[15:0] wdata0;
  logic[15:0] out0;
  bram_256x16 ram0(.clock(clock), .raddr(raddr0), .waddr(waddr0), .wdata(wdata0), .wren(wren), .out(out0));

  logic[7:0]  raddr1;
  logic[7:0]  waddr1;
  logic[15:0] wdata1;
  logic[15:0] out1;
  bram_256x16 ram1(.clock(clock), .raddr(raddr1), .waddr(waddr1), .wdata(wdata1), .wren(wren), .out(out1));

endmodule
*/

//------------------------------------------------------------------------------

/*
module bram_align2_1024
#(
  parameter init_a = "",
  parameter init_b = ""
)
(
  input  logic       clock,
  input  logic[9:0]  raddr,
  output logic[31:0] rdata,
  input  logic[9:0]  waddr,
  input  logic[31:0] wdata,
  input  logic       wren,
);

  always @* begin

    case (raddr[1])
    0: begin raddr_a = (raddr >> 2) + 0; raddr_b = (raddr >> 2) + 0; end
    1: begin raddr_a = (raddr >> 2) + 1; raddr_b = (raddr >> 2) + 0; end
    endcase

    case (waddr[1])
    0: begin waddr_a = (waddr >> 2) + 0; waddr_b = (waddr >> 2) + 0; end
    1: begin waddr_a = (waddr >> 2) + 1; waddr_b = (waddr >> 2) + 0; end
    endcase

    case (waddr[1])
    0: begin wdata_a = wdata[15: 0]; wdata_b = wdata[31:16]; end
    1: begin wdata_a = wdata[31:16]; wdata_b = wdata[15: 0]; end
    endcase

    case (raddr[1])
    0: begin rdata[15: 0] = rdata_a; rdata[31:16] = rdata_b; end
    1: begin rdata[15: 0] = rdata_b; rdata[31:16] = rdata_a; end
    endcase
  end

  logic[7:0]  raddr_a;
  logic[15:0] rdata_a;
  logic[7:0]  waddr_a;
  logic[15:0] wdata_a;
  SB_RAM40_4K ram_a(
    .RDATA(rdata_a), .RCLK(clock), .RCLKE(1), .RE(1), .RADDR(raddr_a),
    .WCLK(clock), .WCLKE(1), .WE(wren), .WADDR(waddr_a), .MASK(16'h0), .WDATA(wdata_a));

  logic[7:0]  raddr_b;
  logic[15:0] rdata_b;
  logic[7:0]  waddr_b;
  logic[15:0] wdata_b;
  SB_RAM40_4K ram_b(
    .RDATA(rdata_b), .RCLK(clock), .RCLKE(1), .RE(1), .RADDR(raddr_b),
    .WCLK(clock), .WCLKE(1), .WE(wren), .WADDR(waddr_b), .MASK(16'h0), .WDATA(wdata_b));

endmodule
*/

//------------------------------------------------------------------------------
// 1k dword-aligned with byte mask

/*
module bram_align2_1024_mask
#(
  parameter init_a = "",
  parameter init_b = ""
)
(
  input  logic       clock,
  input  logic[7:0]  raddr,
  output logic[31:0] rdata,
  input  logic[7:0]  waddr,
  input  logic[31:0] wdata,
  input  logic[3:0]  wmask,
  input  logic       wren,
);

`ifdef SYNTHESIS

  logic[15:0] mask_a;
  logic[15:0] mask_b;

  assign mask_a[ 7: 0] = {8{~wmask[0]}};
  assign mask_a[15: 8] = {8{~wmask[1]}};
  assign mask_b[ 7: 0] = {8{~wmask[2]}};
  assign mask_b[15: 8] = {8{~wmask[3]}};

  SB_RAM40_4K ram_a(
    .RDATA(rdata[15: 0]), .RCLK(clock), .RCLKE(1'b1), .RE(1'b1), .RADDR(11'(raddr)),
    .WCLK(clock), .WCLKE(1'b1), .WE(wren), .WADDR(11'(waddr)), .MASK(mask_a), .WDATA(wdata[15: 0]));

  SB_RAM40_4K ram_b(
    .RDATA(rdata[31:16]), .RCLK(clock), .RCLKE(1'b1), .RE(1'b1), .RADDR(11'(raddr)),
    .WCLK(clock), .WCLKE(1'b1), .WE(wren), .WADDR(11'(waddr)), .MASK(mask_b), .WDATA(wdata[31:16]));

`else

  always_ff @(posedge clock) begin
    if (wren) begin
      if (wmask[0]) ram[waddr][ 7: 0] <= wdata[ 7: 0];
      if (wmask[1]) ram[waddr][15: 8] <= wdata[15: 8];
      if (wmask[2]) ram[waddr][23:16] <= wdata[23:16];
      if (wmask[3]) ram[waddr][31:24] <= wdata[31:24];
    end

    rdata[ 7: 0] <= wren && waddr == raddr ? wdata[ 7: 0] : ram[raddr][ 7: 0];
    rdata[15: 8] <= wren && waddr == raddr ? wdata[15: 8] : ram[raddr][15: 8];
    rdata[23:16] <= wren && waddr == raddr ? wdata[23:16] : ram[raddr][23:16];
    rdata[31:24] <= wren && waddr == raddr ? wdata[31:24] : ram[raddr][31:24];
  end

  logic[31:0] ram[0:255];

`endif

endmodule
*/
