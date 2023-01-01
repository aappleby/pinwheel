Pinwheel # {
  ticks : u64
  code : BlockRam
  data : BlockRam
  regs : BlockRegfile

  vane0 : {
    hart : l5
    pc : l32
    enable : l1
    active : l1
  }

  vane1 : {
    hart : l5
    pc : l32
    insn : l32
    enable : l1
    active : l1
  }

  vane2 : {
    hart : l5
    pc : l32
    insn : l32
    enable : l1
    active : l1
    mem_addr : l32
    alu_out : l32
  }

  debug_reg : l32
}

//--------------------------------------------------------------------------------

void BlockRam::tick_read(logic<32> raddr, logic<1> rden) {
  out @= rden ? data[b10(raddr, 2)] : 0;
}

void BlockRam::tick_write(logic<32> waddr, logic<32> wdata, logic<4> wmask, logic<1> wren) {
  if (wren) {
    old_data := data[b10(waddr, 2)];
    new_data := wdata << (8 * b2(waddr));
    if (!wmask[0]) new_data = (new_data & 0xFFFFFF00) | (old_data & 0x000000FF);
    if (!wmask[1]) new_data = (new_data & 0xFFFF00FF) | (old_data & 0x0000FF00);
    if (!wmask[2]) new_data = (new_data & 0xFF00FFFF) | (old_data & 0x00FF0000);
    if (!wmask[3]) new_data = (new_data & 0x00FFFFFF) | (old_data & 0xFF000000);
    data[b10(waddr, 2)] @= new_data;
  }
}

//--------------------------------------------------------------------------------

void BlockRegfile::tick_read(logic<10> raddr1, logic<10> raddr2, logic<1> rden) {
  out_a @= rden ? data[raddr1] : 0
  out_b @= rden ? data[raddr2] : 0
}

void BlockRegfile::tick_write(logic<10> waddr, logic<32> wdata, logic<1> wren) {
  if (wren) {
    data[waddr] @= wdata;
  }
}

//--------------------------------------------------------------------------------

reset() {
  code.data #= readmemh(value_plusargs("text_file=%s"))
  data.data #= readmemh(value_plusargs("data_file=%s"))

  vane0.pc #= 0x00400000;
  vane1.pc #= 0x00400000;
  vane2.pc #= 0x00400000;

  vane0.hart #= 0;
  vane1.hart #= 1;
  vane2.hart #= 2;

  vane0.enable #= 1;
}

//--------------------------------------------------------------------------------

unpack(insn, addr, data) {
  align := b2(addr);
  f3    := b3(insn, 12);

  return match f3 {
    0 -> signed( b8(data, align << 3), 32)
    1 -> signed(b16(data, align << 3), 32)
    2 -> data
    3 -> data
    4 -> unsigned( b8(data, align << 3), 32)
    5 -> unsigned(b16(data, align << 3), 32)
    6 -> data
    7 -> data
  }
}

//--------------------------------------------------------------------------------

alu(insn : l32, pc : l32, reg_a : l32, reg_b : l32) {
  op  := b5(insn, 2);
  f3  := b3(insn, 12);
  alt := b1(insn, 30);

  imm_i := sign_extend<32>(b12(insn, 20));
  imm_u := b20(insn, 12) :: b12(0);

  alu_a := reg_a
  alu_b := match 1 {
    op == OP_ALU && f3 == 0 && alt -> -reg_b
    op == OP_ALUI -> imm_i
    _ -> reg_b
  }

  alu_out := match f3 {
    0 -> alu_a + alu_b
    1 -> alu_a << b5(alu_b)
    2 -> signed(alu_a) < signed(alu_b)
    3 -> alu_a < alu_b
    4 -> alu_a ^ alu_b
    5 -> alt ? signed(alu_a) >> b5(alu_b) : alu_a >> b5(alu_b)
    6 -> alu_a | alu_b
    7 -> alu_a & alu_b
  }

  return match op {
    OP_ALU   -> alu_out
    OP_ALUI  -> alu_out
    OP_JAL   -> pc + 4
    OP_JALR  -> pc + 4
    OP_LUI   -> imm_u
    OP_AUIPC -> pc + imm_u
    _        -> 0
  }

}

//--------------------------------------------------------------------------------

pc_gen(pc : l32, insn : l32, active : l1, reg_a : l32, reg_b : l32) {
  if (!active) return pc;

  op  := b5(insn, 2);
  eq  := reg_a == reg_b;
  slt := signed(reg_a) < signed(reg_b);
  ult := reg_a < reg_b;
  f3  := b3(insn, 12);

  take_branch := match f3 {
    0 ->   eq
    1 ->  !eq
    2 ->   eq
    3 ->  !eq
    4 ->  slt
    5 -> !slt
    6 ->  ult
    7 -> !ult
    _ ->    0
  }

  imm_b := dup<20>(insn[31]) :: insn[7] :: b6(insn, 25) :: b4(insn, 8) :: b1(0);
  imm_j := dup<12>(insn[31]) :: b8(insn, 12) :: insn[20] :: b10(insn, 21) :: b1(0);
  imm_i := sign_extend<32>(b12(insn, 20));

  return match op {
    OP_BRANCH -> pc + (take_branch ? imm_b : b32(4))
    OP_JAL    -> pc + imm_j
    OP_JALR   -> reg_a + imm_i
    _         -> pc + 4
  }
}

//--------------------------------------------------------------------------------

addr_gen(insn : l32, reg_a : l32) -> l32 {
  op    := b5(insn, 2);
  imm_i := sign_extend<32>(b12(insn, 20));
  imm_s := dup<21>(insn[31]) :: b6(insn, 25) :: b5(insn, 7);
  addr  := reg_a + ((op == OP_STORE) ? imm_s : imm_i);
  return addr;
}

//--------------------------------------------------------------------------------

mask_gen(insn : l32, reg_a : l32) -> l4 {
  op    := b5(insn, 2);
  imm_i := sign_extend<32>(b12(insn, 20));
  imm_s := dup<21>(insn[31]) :: b6(insn, 25) :: b5(insn, 7);
  addr  := reg_a + ((op == OP_STORE) ? imm_s : imm_i);
  f3    := b3(insn, 12);
  align := b2(addr);

  if (op != OP_STORE) return 0;
  return match f3 {
    0 -> 0b0001 << align
    1 -> 0b0011 << align
    2 -> 0b1111
    _ -> 0
  }
}

//--------------------------------------------------------------------------------

tick(reset_in : l1) {
  ticks@ = ticks + 1;

  //----------

  with vane0 {
    // Vane 0 becomes active if vane 2 was set to enable
    hart   @= vane2.hart;
    pc     @= vane2.pc;
    enable @= vane2.enable;
    active @= vane2.enable | vane2.active;

    reg_read := {
      addr1: hart :: b5(code.out, 15)
      addr2: hart :: b5(code.out, 20)
      rden:  active
    }

    code_read := {
      addr : pc,
      rden : active
    }    
  }

  with vane1 {
    reg_a := b5(insn, 15) ? regs.out_a : b32(0); // Mask out r0 if we read it from the regfile.
    reg_b := b5(insn, 20) ? regs.out_b : b32(0); // Mask out r0 if we read it from the regfile.

    op   := b5(insn, 2);
    addr := addr_gen(insn, reg_a);
    mask := mask_gen(insn, reg_a);

    code_cs    := addr[31:28] == 0x0;
    data_cs    := addr[31:28] == 0x8;
    debug_cs   := addr[31:28] == 0xF;
    regfile_cs := addr[31:28] == 0x1;
    
    mem_read := {
      raddr: addr
      rden:  active && op == OP_LOAD
    }

    mem_write := {
      waddr: addr
      wdata: reg_b
      wmask: mask_gen(insn, reg_a)
      wren:  active && op == OP_STORE
    }

    data_read   := mem_read  with rden = rden & data_cs
    data_write  := mem_write with wren = wren & data_cs

    code_read   := mem_read  with rden = rden & code_cs
    code_write  := mem_write with wren = wren & code_cs

    debug_read  := mem_read  with rden = rden & debug_cs
    debug_write := mem_write with wren = wren & debug_cs

    reg_read := {
      raddr1: addr[11:2]
      raddr2: DONTCARE
      rden:   active && op == OP_LOAD && regfile_cs
    }

    reg_write := {
      waddr: addr[11:2]
      wdata: reg_b
      wren:  active && op == OP_STORE && regfile_cs
    }

    // Vane 1 picks up the instruction from the code bus
    hart   @= vane0.hart;
    pc     @= vane0.pc;
    insn   @= code.out;
    enable @= vane0.enable;
    active @= vane0.active;
  }

  with vane2 {
    op := b5(insn, 2);

    reg_write := {
      addr: hart :: b5(insn, 7)
      data: op == OP_LOAD ? bus_rdata : alu_out
      wren: @vane0.active && addr != 0 && op != OP_STORE && op != OP_BRANCH;
    }

    // Vane 2 updates the PC from vane 1
    hart     @= vane1.hart;
    pc       @= pc_gen(vane1); 
    insn     @= vane1.insn;
    enable   @= vane1.enable;
    active   @= vane1.active;
    alu_out  @= alu(vane1);
    mem_addr @= vane1.mem_addr;
  }

  bus_rdata := unpack(
    vane2.insn,
    vane2.mem_addr,
    match vane2.mem_addr[31:28] {
      0x0 -> code.out,
      0x1 -> regs.out_a,
      0x8 -> data.out,
      0xF -> debug_reg.out,
      _   -> DONTCARE,
    }
  );

  data.read  @= vane1.data_read
  data.write @= vane1.data_write

  code.read  @= @vane0.active ? @vane.code_read : vane1.code_read
  code.write @= vane1.code_write

  debug_reg.read  @= vane1.debug_read
  debug_reg.write @= vane1.debug_write

  regs.read @= match(1) {
    vane0.reg_read.rden -> vane0.reg_read
    vane1.reg_read.rden -> vane1.reg_read
    _                   -> { DONTCARE, DONTCARE, false }
  }

  regs.write @= match(1) {
    vane2.reg_write.wren -> vane2.reg_write
    vane1.reg_write.wren -> vane1.reg_write
    _                    -> { DONTCARE, DONTCARE, false }
  }
}

//--------------------------------------------------------------------------------
