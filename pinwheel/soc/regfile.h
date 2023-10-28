#ifndef PINHWEEL_SOC_REGFILE_H
#define PINHWEEL_SOC_REGFILE_H

#include "metron/metron_tools.h"
#include "pinwheel/tools/regfile_if.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal

class regfile {
public:

  regfile() {
    for (int i = 0; i < 256; i++) {
      data1_hi[i] = 0;
      data1_lo[i] = 0;
      data2_hi[i] = 0;
      data2_lo[i] = 0;
    }
  }

  void tick(regfile_in in) {
    if (in.wren) {
      out_1 = in.raddr1 == in.waddr ? in.wdata : cat(data1_hi[in.raddr1], data1_lo[in.raddr1]);
      out_2 = in.raddr2 == in.waddr ? in.wdata : cat(data2_hi[in.raddr2], data2_lo[in.raddr2]);
      data1_hi[in.waddr] = b16(in.wdata, 16);
      data1_lo[in.waddr] = b16(in.wdata, 0);
      data2_hi[in.waddr] = b16(in.wdata, 16);
      data2_lo[in.waddr] = b16(in.wdata, 0);
    }
    else {
      out_1 = cat(data1_hi[in.raddr1], data1_lo[in.raddr1]);
      out_2 = cat(data2_hi[in.raddr2], data2_lo[in.raddr2]);
    }
  }

  logic<32> get_rs1() const { return out_1; }
  logic<32> get_rs2() const { return out_2; }

  /* metron_noconvert */ const uint32_t get(int index) const {
    return cat(data1_hi[index], data1_lo[index]);
  }

  /* metron_internal */ logic<16> data1_hi[256];
  /* metron_internal */ logic<16> data1_lo[256];
  /* metron_internal */ logic<16> data2_hi[256];
  /* metron_internal */ logic<16> data2_lo[256];
  /* metron_internal */ logic<32> out_1;
  /* metron_internal */ logic<32> out_2;
};

//------------------------------------------------------------------------------
// verilator lint_on unusedsignal

#endif // PINHWEEL_SOC_REGFILE_H
