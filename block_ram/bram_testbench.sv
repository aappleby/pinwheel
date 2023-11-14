`include "bram.sv"
`timescale 1ns/100ps

module bram_testbench();

  always #5 clock = ~clock;

  initial begin
    $display("Hello World\n");

    clock = 0;
    raddr = 0;
    waddr = 0;
    wdata = 0;
    wren  = 0;
    #1

    raddr = 0;
    waddr = 0;
    wdata = 32'h00000000;
    wren  = 1;
    #10 $display("%x", out);

    raddr = 4;
    waddr = 4;
    wdata = 32'h00000000;
    wren  = 1;
    #10 $display("%x", out);

    raddr = 3;
    waddr = 3;
    wdata = 32'h12345678;
    wren  = 1;
    #10 $display("%x", out);

    $display();


    raddr = 4;
    wren = 0;
    #10 $write("%x", out);

    raddr = 0;
    wren = 0;
    #10 $write("%x", out);

    $display();
    $finish();
  end

  logic       clock;
  logic[10:0] raddr;
  logic[10:0] waddr;
  logic[31:0] wdata;
  logic       wren;
  logic[31:0] out;
  bram_unaligned dut(.clock(clock), .raddr(raddr), .waddr(waddr), .wdata(wdata), .wren(wren), .out(out));

  initial begin
    $dumpfile("dump.vcd");
    $dumpvars(0, bram_testbench);
  end

endmodule
