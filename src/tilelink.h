#pragma once
#include "metron_tools.h"

struct TileLinkA {
  logic<3>  a_opcode;
  logic<3>  a_param;
  logic<3>  a_size;
  logic<1>  a_source;
  logic<32> a_address;
  logic<4>  a_mask;
  logic<32> a_data;
  logic<1>  a_valid;
  logic<1>  a_ready;
};

struct TileLinkD {
  logic<3>  d_opcode;
  logic<2>  d_param;
  logic<3>  d_size;
  logic<1>  d_source;
  logic<3>  d_sink;
  logic<32> d_data;
  logic<1>  d_error;
  logic<1>  d_valid;
  logic<1>  d_ready;
};
