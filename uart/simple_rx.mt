// Simple 8n1 UART receiver in Metron

param clocks_per_bit = 4
param bits_per_byte  = 10

param delay_mid = clocks_per_bit / 2
param delay_max = clocks_per_bit - 1
param count_max = bits_per_byte - 1

ser_in : sig[1]  = input
delay  : reg[8]  = delay_max
count  : reg[8]  = count_max
shift  : reg[8]  = 0
out    : sig[8]  = @shift
valid  : sig[1]  = 0


if delay < delay_max
  @delay = delay + 1
else if count < count_max
  @delay = 0
  @count = count + 1
else if ser_in == 0
  @delay = 0
  @count = 0

if delay == delay_mid
  @shift = (ser_in :: shift) >> 1
  valid = count == 8
