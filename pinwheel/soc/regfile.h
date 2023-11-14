#ifndef PINHWEEL_SOC_REGFILE_H
#define PINHWEEL_SOC_REGFILE_H

#include "metron/metron_tools.h"
#include "pinwheel/tools/regfile_if.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedsignal

class regfile {
public:

  logic<32> get_rs1() const { return out_1; }
  logic<32> get_rs2() const { return out_2; }

  void tock(regfile_if in) {
    tick(in);
  }

  /* metron_noconvert */
  const uint32_t get(int index) const {
    return data1[index];
  }

private:

  void tick(regfile_if in) {
    if (in.wren) {
      out_1 = in.raddr1 == in.waddr ? in.wdata : data1[in.raddr1];
      out_2 = in.raddr2 == in.waddr ? in.wdata : data2[in.raddr2];
      data1[in.waddr] = in.wdata;
      data2[in.waddr] = in.wdata;
    }
    else {
      out_1 = data1[in.raddr1];
      out_2 = data2[in.raddr2];
    }
  }

  logic<32> data1[256];
  logic<32> data2[256];
  logic<32> out_1;
  logic<32> out_2;
};

//------------------------------------------------------------------------------
// verilator lint_on unusedsignal

#endif // PINHWEEL_SOC_REGFILE_H
