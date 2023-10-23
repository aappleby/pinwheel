`ifndef PINWHEEL_RTL_CONSOLE_H
`define PINWHEEL_RTL_CONSOLE_H

`include "metron/metron_tools.sv"
`include "pinwheel/metron/tilelink.sv"

//------------------------------------------------------------------------------
// verilator lint_off unusedparam

module console (
  // global clock
  input logic clock,
  // tick() ports
  input logic tick_reset,
  input tilelink_a tick_tla
);
  parameter addr_mask = 32'hF0000000;
  parameter addr_tag = 32'h00000000;
/*public:*/

  always_ff @(posedge clock) begin : tick
    /*
    if (reset) {
      memset(buf, 0, sizeof(buf));
      x = 0;pinwheel
      y = 0;
    }
    else {

      if (((tla.a_address & addr_mask) == addr_tag) && (tla.a_opcode == TL::PutPartialData)) {
        buf[y * width + x] = 0;
        auto c = char(tla.a_data);

        if (c == 0) c = '?';

        if (c == '\n') {
          x = 0;
          y++;
        }
        else if (c == '\r') {
          x = 0;
        }
        else {
          buf[y * width + x] = c;
          x++;
        }

        if (x == width) {
          x = 0;
          y++;
        }
        if (y == height) {
          memcpy(buf, buf + width, width*(height-1));
          memset(buf + (width*(height-1)), 0, width);
          y = height-1;
        }
        buf[y * width + x] = 30;
      }
    }
    */
  end

  //static const int width =64;
  //static const int height=16;

  //char buf[width*height];
  //int  x = 0;
  //int  y = 0;
endmodule

// verilator lint_on unusedparam
//------------------------------------------------------------------------------

`endif // PINWHEEL_RTL_CONSOLE_H
