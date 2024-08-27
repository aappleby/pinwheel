# UART demo toplevel

[types]
simple_msg  = load("simple_msg.mt")
simple_tx   = load("simple_tx.mt")
simple_rx   = load("simple_rx.mt")
simple_sink = load("simple_sink.mt")

[params]
clocks_per_bit = 1

[state]
msg  : simple_message
tx   : simple_tx(clocks_per_bit)
rx   : simple_rx(clocks_per_bit)
sink : simple_sink

[update]
msg.out <> tx.in
tx.out  <> rx.in
rx.out  <> sink.in
