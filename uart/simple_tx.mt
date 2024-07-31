// Simple 8n1 UART transmitter in Metron

param clocks_per_bit = 4
param bits_per_byte  = 10

param delay_mid = clocks_per_bit / 2
param delay_max = clocks_per_bit - 1
param count_max = bits_per_byte - 1

in       : sig[8] = input
in_valid : sig[1] = input
in_ready : sig[1] = 0
delay    : reg[8] = delay_max
count    : reg[8] = count_max
shift    : reg[8] = dontcare
out      : sig[1] = shift[0]

if delay < delay_max
  @delay = delay + 1
else if count < count_max
  @delay = 0
  @count = count + 1
else if in_valid
  @delay = 0
  @count = 0

when delay == delay_max && count < count_max
  @shift = (b1 :: shift) >> 1

when delay == delay_max && count == count_max
  in_ready = 1
  if in_valid
    @shift = b1 :: in :: b0
