# Simple message transmitter with a delay between transmissions

[params]
max_delay  = 20;
max_cursor = len(text) - 1;

[ports]
out.data  :> u8 = text[cursor];
out.valid :> u1 = delay == 0;
out.ready <: u1;

[types]
reg_delay  = unsigned(max_delay);
reg_cursor = unsigned(max_cursor);

[state]
text   : u8[] = read("ping.hex");
delay  : reg_delay  = max_delay;
cursor : reg_cursor = 0;

[update]
match (true) {
  case (delay > 0) {
    @delay = delay - 1;
  }
  case (out.ready) {
    if (cursor == max_cursor) {
      @delay  = max_delay;
      @cursor = 0;
    } else {
      @cursor = cursor + 1;
    }
  }
}
