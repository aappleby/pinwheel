// Simple 8n1 UART receiver in Metron

[params]
clocks_per_bit = 4;
bits_per_byte = 10;
delay_mid = clocks_per_bit / 2;
delay_max = clocks_per_bit - 1;
count_max = bits_per_byte - 1;

[types]
delay_reg = unsigned(max = delay_max);
count_reg = unsigned(max = count_max);
shift_reg = u8;

[ports]
in <: u1;

out.data  :> shift_reg = shift;
out.valid :> u1 = 0;
out.ready <: u1;

[state]
delay : delay_reg = delay_max;
count : count_reg = count_max;
shift : shift_reg = 0;

[update]
match (true) {
  case (delay < delay_max) {
    @delay = delay + 1;
  }
  case (count < count_max) {
    @delay = 0;
    @count = count + 1;
  }
  case (in == 0) {
    @delay = 0;
    @count = 0;
  }
}

if (delay == delay_mid) {
  @shift = in :: (shift >> 1);
  # I think this is wrong...
  out.valid = @count == 8;
}
