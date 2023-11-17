`include "cells_sim.v"
`include "bram.sv"
`timescale 1ns/100ps

module testbench();

  initial begin
    $dumpfile("testbench.vcd");
    $dumpvars(0, testbench);
  end


  logic clock = 0;
  always #5 clock = ~clock;

  logic[8:0] raddr;
  logic[7:0] rdata;
  logic[8:0] waddr;
  logic[7:0] wdata;
  logic      wren;
  bram_512x8 ram(.clock(clock), .raddr(raddr), .rdata(rdata), .waddr(waddr), .wdata(wdata), .wren(wren));

  task do_mem(logic[8:0] _raddr, logic[8:0] _waddr, logic[7:0] _wdata, logic _wren);
    raddr = _raddr;
    waddr = _waddr;
    wdata = _wdata;
    wren  = _wren;
    #10;
  endtask

  initial begin
    int i, j, k;

    for (i = 0; i < 512; i++) begin
      do_mem(0, i, i + 70, 1);
    end

    for (j = 0; j < 32; j++) begin
      for (i = 0; i < 16; i++) begin
        do_mem(j * 16 + i, 0, 0, 0);
        $write("%02x ", rdata);
      end
      $write("\n");
    end
    $write("\n");

    assert(ram.

    $finish();
  end

  /*
  task do_mem(logic[10:0] _raddr, logic[10:0] _waddr, logic[31:0] _wdata, logic[1:0] _wsize, logic _wren);
    raddr = _raddr;
    waddr = _waddr;
    wdata = _wdata;
    wsize = _wsize;
    wren  = _wren;
    assert(0 == 0);
    #10;
  endtask

  task test_mem();
    int i, j, k;

    for (int j = 0; j < 16; j++) begin
      for (int i = 0; i < 4; i++) begin
        do_mem(j * 16 + i * 4, 0, 0, 0, 0);
        $write("%02x ", out[7:0]);
        $write("%02x ", out[15:8]);
        $write("%02x ", out[23:16]);
        $write("%02x ", out[31:24]);
      end
      $write("\n");
    end
    $write("\n");
  endtask;

  initial begin
    int offset;
    int i, j, k;

    $dumpfile("testbench.vcd");
    $dumpvars(0, testbench);

    clock = 0;
    raddr = 0;
    waddr = 0;
    wdata = 0;
    wren  = 0;

    for (i = 0; i < 16; i++) begin
      do_mem(0, i * 17, 32'h12345678, 3, 1);
    end

    test_mem();

    $finish();
  end

  logic[10:0] raddr;
  logic[10:0] waddr;
  logic[31:0] wdata;
  logic[1:0]  wsize;
  logic       wren;
  logic[31:0] out;
  bram_align1_2048 dut(.clock(clock), .raddr(raddr), .waddr(waddr), .wdata(wdata), .wsize(wsize), .wren(wren), .out(out));
  */

  /*
  logic[9:0]  raddr;
  logic[9:0]  waddr;
  logic[31:0] wdata;
  logic[1:0]  wsize;
  logic       wren;
  logic[31:0] out;
  bram_align2_1024 dut(.clock(clock), .raddr(raddr), .waddr(waddr), .wdata(wdata), .wsize(wsize), .wren(wren), .out(out));
  */


endmodule
