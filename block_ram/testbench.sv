`include "bram.sv"
`timescale 1ns/100ps

module testbench();

  task test_mem();
    int i, j, k;

    for (int j = 0; j < 16; j++) begin
      for (int i = 0; i < 4; i++) begin
        do_mem(j * 16 + i * 4, 0, 0, 0);
        $write("%02x ", out[7:0]);
        $write("%02x ", out[15:8]);
        $write("%02x ", out[23:16]);
        $write("%02x ", out[31:24]);
      end
      $write("\n");
    end
    $write("\n");
  endtask;

  task do_mem(logic[10:0] _raddr, logic[10:0] _waddr, logic[31:0] _wdata, logic _wren);
    raddr = _raddr;
    waddr = _waddr;
    wdata = _wdata;
    wren  = _wren;
    assert(0 == 0);
    #10;
  endtask

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
      do_mem(0, i * 17, 32'h12345678, 1);
    end

    test_mem();

    $finish();
  end

  logic       clock;
  logic[10:0] raddr;
  logic[10:0] waddr;
  logic[31:0] wdata;
  logic       wren;
  logic[31:0] out;
  bram_align1_2048 dut(.clock(clock), .raddr(raddr), .waddr(waddr), .wdata(wdata), .wren(wren), .out(out));

  always #5 clock = ~clock;

endmodule
