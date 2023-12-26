#ifndef PINHWEEL_SOC_REGFILE_H
#define PINHWEEL_SOC_REGFILE_H

#include "metron/metron_tools.h"
#include "pinwheel/tools/regfile_if.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal

// 8 kilobits of ram in 2 SB_RAM40_4K blocks

class ram_256x32 {
public:

  ram_256x32(const char* filename = nullptr) {
    if (filename) {
      readmemh(filename, ram_);
    }
  }

  // metron_noemit
  void tick(logic<8> raddr, logic<8> waddr, logic<32> wdata, logic<1> wren) {
    rdata_ = raddr == waddr ? wdata : ram_[raddr];
    if (wren) ram_[waddr] = wdata;
  }

  // metron_noemit
  logic<32> ram_[256];

  logic<32> rdata_;

  /*#
  SB_RAM40_4K #(
		.READ_MODE(0),
		.WRITE_MODE(0)
	)
  my_ram_lo (
    .RCLK(clock), .RADDR({3'b0, tick_raddr}), .RDATA(rdata_[15:0]),
    .WCLK(clock), .WADDR({3'b0, tick_waddr}), .WDATA(tick_wdata[15:0]), .WE(tick_wren),
  );

  SB_RAM40_4K #(
		.READ_MODE(0),
		.WRITE_MODE(0)
	)
  my_ram_hi (
    .RCLK(clock), .RADDR({3'b0, tick_raddr}), .RDATA(rdata_[31:16]),
    .WCLK(clock), .WADDR({3'b0, tick_waddr}), .WDATA(tick_wdata[31:16]), .WE(tick_wren),
  );
  #*/
};

//------------------------------------------------------------------------------

class regfile {
public:

  logic<32> get_rs1() const { return ram1.rdata_; }
  logic<32> get_rs2() const { return ram2.rdata_; }

  // metron_noconvert
  const uint32_t get(int index) const {
    return ram1.rdata_[index];
  }

  void tick(regfile_if in) {
    ram1.tick(b8(in.raddr1), b8(in.waddr), in.wdata, in.wren);
    ram2.tick(b8(in.raddr2), b8(in.waddr), in.wdata, in.wren);
  }

private:

  ram_256x32 ram1;
  ram_256x32 ram2;
};

// verilator lint_on unusedsignal
//------------------------------------------------------------------------------

#endif // PINHWEEL_SOC_REGFILE_H
