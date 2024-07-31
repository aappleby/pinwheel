// Simple 8n1 UART transmitter in Metron

param clocks_per_bit = 4

const bits_per_byte = 10
const delay_mid = clocks_per_bit / 2
const delay_max = clocks_per_bit - 1
const count_max = bits_per_byte - 1

$in       : u8  = input
$in_valid : u1  = input
$in_ready : u1  = 0
@delay    : u8  = delay_max
@count    : u8  = count_max
@shift    : u10 = dontcare
$ser_out  : u1  = shift[0]

if delay < delay_max
  @delay = delay + 1
else if count < count_max
  @delay = 0
  @count = count + 1
  @shift = (b1 :: shift) >> 1
else
  $in_ready = 1
  if $in_valid
    @delay = 0
    @count = 0
    @shift = b1 :: $in :: b0
