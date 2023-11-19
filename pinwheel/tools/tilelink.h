#ifndef PINWHEEL_RTL_TILELINK_H
#define PINWHEEL_RTL_TILELINK_H

#include "metron/metron_tools.h"

//------------------------------------------------------------------------------
// verilator lint_off unusedparam

namespace TL {
  static const int PutFullData = 0;
  /*
  a_opcode  C 3  Must be PutFullData (0).
  a_param   C 3  Reserved; must be 0.
  a_size    C z  2^n bytes will be written by the slave.
  a_source  C o  The master source identifier issuing this request.
  a_address C a  The target address of the Access, in bytes.
  a_mask    D w  Byte lanes to be written; must be contiguous.
  a_data    D 8w Data payload to be written.
  */


  static const int PutPartialData = 1;
  /*
  a_opcode  C 3  Must be PutPartialData (1).
  a_param   C 3  Reserved; must be 0.
  a_size    C z  Up to 2^n bytes will be written by the slave.
  a_source  C o  The master source identifier issuing this request.
  a_address C a  The target base address of the Access, in bytes.
  a_mask    D w  Byte lanes to be written.
  a_data    D 8w Data payload to be written.
  */

  static const int ArithmeticData = 2;
  static const int LogicalData = 3;

  static const int Get = 4;
  /*
  a_opcode  C 3  Must be Get (4).
  a_param   C 3  Reserved; must be 0.
  a_size    C z  2^n bytes will be read by the slave and returned.
  a_source  C o  The master source identifier issuing this request.
  a_address C a  The target address of the Access, in bytes.
  a_mask    D w  Byte lanes to be read from.
  a_data    D 8w Ignored; can be any value.
  */

  static const int Intent = 5;
  static const int Acquire = 6;

  //----------------------------------------

  static const int AccessAck = 0;
  /*
  d_opcode  C 3  Must be AccessAck (0).
  d_param   C 2  Reserved; must be 0.
  d_size    C z  2^n bytes were accessed by the slave.
  d_source  C o  The master source identifier receiving this response.
  d_sink    C i  Ignored; can be any value.
  d_data    D 8w Ignored; can be any value.
  d_error   F 1  The slave was unable to service the request.
  */

  static const int AccessAckData = 1;
  /*
  d_opcode  C 3  Must be AccessAckData (1).
  d_param   C 2  Reserved; must be 0.
  d_size    C z  2^n bytes were accessed by the slave.
  d_source  C o  The master source identifier receiving this response.
  d_sink    C i  Ignored; can be any value.
  d_data    D 8w The data payload.
  d_error   F 1  The slave was unable to service the request.
  */

  static const int HintAck = 2;
  static const int Grant = 4;
  static const int GrantData = 5;
  static const int ReleaseAck = 6;


  static const int Invalid = 7;
};

/*
*/

//----------------------------------------

struct tilelink_a {
  // FIXME enums inside structs are broken in Metron
  logic<3>  a_opcode;
  logic<3>  a_param;
  logic<3>  a_size;
  logic<1>  a_source;
  logic<32> a_address;
  logic<4>  a_mask;
  logic<32> a_data;
  logic<1>  a_valid;
  logic<1>  a_ready; // reverse channel
};

//----------------------------------------

struct tilelink_d {
  logic<3>  d_opcode;
  logic<2>  d_param;
  logic<3>  d_size;
  logic<1>  d_source;
  logic<3>  d_sink;
  logic<32> d_data;
  logic<1>  d_error;
  logic<1>  d_valid;
  logic<1>  d_ready; // reverse channel
};

// verilator lint_on unusedparam
//------------------------------------------------------------------------------

#endif // PINWHEEL_RTL_TILELINK_H
