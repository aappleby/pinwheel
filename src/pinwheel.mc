
void tick(logic<1> reset_in) {
  if (reset_in) {
    reset();
    return;
  }

  cpu_to_mem sig_dbus = vane1.active ? dbus_out(vane1.hart, vane1.insn, ra, rb) : cpu_to_mem{0};

  logic<32> new_data = data_mem[b10(sig_dbus.addr, 2)];
  if (sig_dbus.mask) {
    if (sig_dbus.addr != 0x40000000) {
      if (sig_dbus.mask[0]) new_data = (new_data & 0xFFFFFF00) | (sig_dbus.data & 0x000000FF);
      if (sig_dbus.mask[1]) new_data = (new_data & 0xFFFF00FF) | (sig_dbus.data & 0x0000FF00);
      if (sig_dbus.mask[2]) new_data = (new_data & 0xFF00FFFF) | (sig_dbus.data & 0x00FF0000);
      if (sig_dbus.mask[3]) new_data = (new_data & 0x00FFFFFF) | (sig_dbus.data & 0xFF000000);
      data_mem@[b10(sig_dbus.addr, 2)] = new_data;
    }
  }

  vane0@        = vane2;
  vane1@        = vane0;
  vane2@        = vane1;
  vane0@.active = vane2.enable | vane2.active;
  vane1@.insn   = pbus_data;
  vane2@.pc     = next_pc(vane1.pc, vane1.insn, ra, rb);
  alu_out@      = alu(vane1.pc, vane1.insn, ra, rb);
  align@        = b2(sig_dbus.addr);
  dbus_data@    = new_data;
  pbus_data@    = code_mem[b10(vane2.pc, 2)];;

  auto waddr    = b5(vane2.insn, 7);
  auto write_op = b5(vane2.insn, 2);

  if (vane2.active && waddr && write_op != OP_STORE && write_op != OP_BRANCH) {
    regfile[cat(vane2.hart, waddr)]@ = write_op == OP_LOAD ? unpack(vane2.insn, align, dbus_data) : alu_out;
  }

  ra@ = regfile[cat(vane0.hart, b5(pbus_data, 15))];
  rb@ = regfile[cat(vane0.hart, b5(pbus_data, 20))];
}
