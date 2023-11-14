//------------------------------------------------------------------------------

module bram_512x8
(
  input  logic      clock,
  input  logic[8:0] raddr,
  input  logic[8:0] waddr,
  input  logic[7:0] wdata,
  input  logic      wren,
  output logic[7:0] out
);

  always @(posedge clock) begin
    if (wren) begin
      data[waddr] <= wdata;
    end
    out <= waddr == raddr && wren ? wdata : data[raddr];
  end

  logic[7:0] data[0:511];

endmodule

//------------------------------------------------------------------------------

module bram_unaligned
(
  input  logic       clock,
  input  logic[10:0] raddr,
  input  logic[10:0] waddr,
  input  logic[31:0] wdata,
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

    case (waddr[1:0])
    2'b00: begin wdata0 = wdata[7:0]; wdata1 = wdata[15:8]; wdata2 = wdata[23:16]; wdata3 = wdata[31:24]; end
    2'b01: begin wdata1 = wdata[7:0]; wdata2 = wdata[15:8]; wdata3 = wdata[23:16]; wdata0 = wdata[31:24]; end
    2'b10: begin wdata2 = wdata[7:0]; wdata3 = wdata[15:8]; wdata0 = wdata[23:16]; wdata1 = wdata[31:24]; end
    2'b11: begin wdata3 = wdata[7:0]; wdata0 = wdata[15:8]; wdata1 = wdata[23:16]; wdata2 = wdata[31:24]; end
    endcase

    wren0 = wren;
    wren1 = wren;
    wren2 = wren;
    wren3 = wren;

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

//------------------------------------------------------------------------------
