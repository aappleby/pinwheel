[params]
plug : type

[ports]
A <: plug
B <: plug
C :> plug

[update]
C = A & B






[types]
and_gate = load("and_gate.mt")

[state]
and_gate_u8 = and_gate(u8)
