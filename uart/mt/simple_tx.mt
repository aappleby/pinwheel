// Simple 8n1 UART transmitter in Metron

[params]
clocks_per_bit = 4
bits_per_byte = 10
delay_mid = clocks_per_bit / 2
delay_max = clocks_per_bit - 1
count_max = bits_per_byte - 1

[ports]
in.data  <: u8
in.valid <: u1
in.ready :> u1 = 0
out.data :> u1 = shift[0]

[types]
reg_delay = unsigned(delay_max)
reg_count = unsigned(count_max)
reg_shift = u10

[regs]
delay : reg_delay = delay_max
count : reg_count = count_max
shift : reg_shift = dontcare

[update]
match true:
  case delay < delay_max:
    @delay = delay + 1
  case count < count_max:
    @delay = 0
    @count = count + 1
    @shift = 1u1 :: (shift >> 1)
  default:
    in.ready = 1
    if in.valid
      @delay = 0
      @count = 0
      @shift = 1u1 :: in.data :: 0u1
