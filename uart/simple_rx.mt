// Simple 8n1 UART receiver in Metron

param clocks_per_bit = 4

const bits_per_byte = 10
const delay_mid = clocks_per_bit / 2
const delay_max = clocks_per_bit - 1
const count_max = bits_per_byte - 1

$ser_in    : u1  = input
@delay     : u8  = delay_max
@count     : u8  = count_max
@shift     : u8  = 0
$out       : u8  = @shift
$out_valid : u1  = 0
$out_ready : u1  = 0

if delay < delay_max
  @delay = delay + 1
else if count < count_max
  @delay = 0
  @count = count + 1
else if ser_in == 0
  @delay = 0
  @count = 0

if delay == delay_mid
  @shift = ($ser_in :: shift) >> 1
  $out_valid = count == 8
