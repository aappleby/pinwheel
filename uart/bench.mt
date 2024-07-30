
//------------------------------------------------------------------------------

ports:

  wire rx_in

//------------------------------------------------------------------------------

params:
  localparam clocks_per_bit = 2;
  localparam bits_per_byte = 10;

  localparam delay_mid = clocks_per_bit / 2;
  localparam delay_max = clocks_per_bit - 1;
  localparam count_max = bits_per_byte - 1;

//------------------------------------------------------------------------------

init:
  logic<8> rx_delay = delay_max
  logic<8> rx_count = count_max

  logic<8> rx_shift = 0
  logic<8> rx_data  = 0
  logic    rx_valid = 0

//------------------------------------------------------------------------------

default:
  rx_valid@ ~= 0 // weak assign

//------------------------------------------------------------------------------

update:
  if rx_delay < delay_max:
    rx_delay@ = rx_delay + 1
  else if rx_count < count_max:
    rx_delay@ = 0
    rx_count@ = rx_count + 1
  else if rx_in == 0:
    rx_delay@ = 0
    rx_count@ = 0

  if rx_delay@ == delay_mid:
    rx_shift@ = {rx_in, rx_shift[7:1]}
    if rx_count@ == count_max - 1:
      rx_valid@ = 1
      rx_data@  = rx_shift@

//------------------------------------------------------------------------------
