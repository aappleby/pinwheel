// Simple byte sink with ready/valid input

[ports]
in.data  <: u8;
in.valid <: u1;
in.ready :> u1 = 1;

[state]
data : u8 = 0;

[update]
if (in.valid) {
  @data = in.data;
}
