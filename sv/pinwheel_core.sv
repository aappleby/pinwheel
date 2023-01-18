`include "metron_tools.sv"

`include "regfile.sv"

// Address Map
// 0x0xxxxxxx - Code
// 0x8xxxxxxx - Data
// 0xExxxxxxx - Regfiles
// 0xFxxxxxxx - Debug registers

module pinwheel_core (
  // global clock
  input logic clock,
  // output signals
  output logic[31:0] sig_code_addr,
  output logic[31:0] sig_code_wdata,
  output logic[3:0]  sig_code_wmask,
  output logic  sig_code_wren,
  output logic[31:0] sig_bus_addr,
  output logic[31:0] sig_bus_wdata,
  output logic[3:0]  sig_bus_wmask,
  output logic  sig_bus_wren,
  // tock() ports
  input logic[31:0] tock_code_rdata,
  input logic[31:0] tock_bus_rdata,
  // tick() ports
  input logic tick_reset_in
);
/*public:*/

  //----------------------------------------

  always_comb begin : tock
    logic[4:0]  rs1a_a;
    logic[4:0]  rs2a_a;
    logic[9:0] reg_raddr1_a;
    logic[9:0] reg_raddr2_a;
    logic[4:0]  op_b;
    logic[2:0]  f3_b;
    logic[4:0]  rs1a_b;
    logic[4:0]  rs2a_b;
    logic[31:0] rs1_b;
    logic[31:0] rs2_b;
    logic[31:0] imm_b;
    logic  regfile_cs_b;
    logic[11:0] csr_b;
    logic[4:0]  op_c;
    logic[4:0]  rd_c;
    logic[2:0]  f3_c;
    logic[11:0] csr_c;
    logic[3:0]  bus_tag_c;
    logic  regfile_cs_c;
    logic[31:0] data_out_c;
    logic[31:0] temp_result_c;
    logic[31:0] next_hpc;
    logic[31:0] alu_result;

    sig_insn_a  = 24'(hpc_a) ? tock_code_rdata : 32'd0;
    rs1a_a  = sig_insn_a[19:15];
    rs2a_a  = sig_insn_a[24:20];
    reg_raddr1_a = {hpc_a[28:24], rs1a_a};
    reg_raddr2_a = {hpc_a[28:24], rs2a_a};

    op_b   = insn_b[6:2];
    f3_b   = insn_b[14:12];
    rs1a_b = insn_b[19:15];
    rs2a_b = insn_b[24:20];
    rs1_b  = rs1a_b ? regs_get_rs1_ret : 32'd0;
    rs2_b  = rs2a_b ? regs_get_rs2_ret : 32'd0;
    imm_b  = decode_imm(insn_b);
    sig_addr_b  = 32'(rs1_b + imm_b);
    regfile_cs_b = sig_addr_b[31:28] == 4'hE;
    csr_b = insn_b[31:20];

    op_c  = insn_c[6:2];
    rd_c  = insn_c[11:7];
    f3_c  = insn_c[14:12];
    csr_c = insn_c[31:20];
    bus_tag_c    = addr_c[31:28];
    regfile_cs_c = bus_tag_c == 4'hE;
    data_out_c   = regfile_cs_c ? regs_get_rs1_ret : tock_bus_rdata;

    temp_result_c = result_c;

    //----------
    // Fetch

    next_hpc = 0;
    begin
      logic take_branch;
      take_branch = 0;
      if (24'(hpc_b)) begin
        logic eq;
        logic slt;
        logic ult;
        eq  = rs1_b == rs2_b;
        slt = $signed(rs1_b) < $signed(rs2_b);
        ult = rs1_b < rs2_b;

        case (f3_b)
          0:  take_branch =   eq;
          1:  take_branch =  !eq;
          2:  take_branch =   eq;
          3:  take_branch =  !eq;
          4:  take_branch =  slt;
          5:  take_branch = !slt;
          6:  take_branch =  ult;
          7:  take_branch = !ult;
          default: take_branch =    0;
        endcase
      end

      if (24'(hpc_b)) begin
        case(op_b)
          RV32I::OP_BRANCH: next_hpc = take_branch ? hpc_b + imm_b : hpc_b + 4;
          RV32I::OP_JAL:    next_hpc = hpc_b + imm_b;
          RV32I::OP_JALR:   next_hpc = sig_addr_b;
          RV32I::OP_LUI:    next_hpc = hpc_b + 4;
          RV32I::OP_AUIPC:  next_hpc = hpc_b + 4;
          RV32I::OP_LOAD:   next_hpc = hpc_b + 4;
          RV32I::OP_STORE:  next_hpc = hpc_b + 4;
          RV32I::OP_SYSTEM: next_hpc = hpc_b + 4;
          RV32I::OP_OPIMM:  next_hpc = hpc_b + 4;
          RV32I::OP_OP:     next_hpc = hpc_b + 4;
        endcase
      end
    end

    //----------
    // Execute

    alu_result = 0;
    begin
      case(op_b)
        RV32I::OP_BRANCH: alu_result = 32'bx;
        RV32I::OP_JAL:    alu_result = hpc_b + 4;
        RV32I::OP_JALR:   alu_result = hpc_b + 4;
        RV32I::OP_LUI:    alu_result = imm_b;
        RV32I::OP_AUIPC:  alu_result = hpc_b + imm_b;
        RV32I::OP_LOAD:   alu_result = sig_addr_b;
        RV32I::OP_STORE:  alu_result = rs2_b;
        RV32I::OP_SYSTEM: alu_result = execute_system(insn_b, rs1_b, rs2_b);
        RV32I::OP_OPIMM:  alu_result = execute_alu   (insn_b, rs1_b, rs2_b);
        RV32I::OP_OP:     alu_result = execute_alu   (insn_b, rs1_b, rs2_b);
        default:               alu_result = 32'bx;
      endcase

      if (op_b == RV32I::OP_SYSTEM && f3_b == RV32I::F3_CSRRW && csr_b == 12'h801) begin
        logic[31:0] temp;
        temp = alu_result;
        alu_result = next_hpc;
        next_hpc = temp;
      end

      if (op_c == RV32I::OP_SYSTEM && f3_c == RV32I::F3_CSRRW && csr_c == 12'h800) begin
        logic[31:0] temp;
        temp = temp_result_c;
        temp_result_c = next_hpc;
        next_hpc = temp;
      end

    end
    sig_next_hpc_a = next_hpc;
    sig_result_b = alu_result;

    //----------
    // Memory: Data bus

    begin
      logic[3:0]          temp_mask_b;
      temp_mask_b = 0;
      if (f3_b == 0)    temp_mask_b = 4'b0001;
      if (f3_b == 1)    temp_mask_b = 4'b0011;
      if (f3_b == 2)    temp_mask_b = 4'b1111;
      if (sig_addr_b[0]) temp_mask_b = temp_mask_b << 1;
      if (sig_addr_b[1]) temp_mask_b = temp_mask_b << 2;

      sig_bus_addr   = sig_addr_b;
      sig_bus_wdata  = rs2_b;
      sig_bus_wmask  = temp_mask_b;
      sig_bus_wren   = (op_b == RV32I::OP_STORE);
    end

    //----------
    // Memory + code/data/reg read/write overrides for cross-thread stuff

    begin
      logic[3:0]       temp_mask_c;
      logic code_cs_c;
      // We write code memory in phase C because it's busy reading the next
      // instruction in phase B.

      // Hmm we can't actually read from code because we also have to read our next instruction
      // and we can't do it earlier or later (we can read it during C, but then it's not back
      // in time to write to the regfile).

      temp_mask_c = 0;
      if (f3_c == 0) temp_mask_c = 4'b0001;
      if (f3_c == 1) temp_mask_c = 4'b0011;
      if (f3_c == 2) temp_mask_c = 4'b1111;
      if (addr_c[0]) temp_mask_c = temp_mask_c << 1;
      if (addr_c[1]) temp_mask_c = temp_mask_c << 2;

      code_cs_c = bus_tag_c == 4'h0 && 24'(sig_next_hpc_a) == 0;

      sig_code_addr  = code_cs_c ? 24'(addr_c) : 24'(sig_next_hpc_a);
      sig_code_wdata = temp_result_c;
      sig_code_wmask = temp_mask_c;
      sig_code_wren  = (op_c == RV32I::OP_STORE) && code_cs_c;
    end

    //----------
    // Regfile write

    begin
      logic[31:0] unpacked_c;
      logic[9:0] temp_wb_addr_c;
      unpacked_c = data_out_c;
      if (temp_result_c[0]) unpacked_c = unpacked_c >> 8;
      if (temp_result_c[1]) unpacked_c = unpacked_c >> 16;
      case (f3_c)
        0:  unpacked_c = $signed( 8'(unpacked_c));
        1:  unpacked_c = $signed(16'(unpacked_c));
        4:  unpacked_c = $unsigned( 8'(unpacked_c));
        5:  unpacked_c = $unsigned(16'(unpacked_c));
      endcase

      // If we're using jalr to jump between threads, we use the hart from HPC _A_
      // as the target for the write so that the link register will be written
      // in the _destination_ regfile.

      temp_wb_addr_c = {op_c == RV32I::OP_JALR ? hpc_a : hpc_c[28:24], rd_c};
      sig_wb_data_c = op_c == RV32I::OP_LOAD ? unpacked_c : temp_result_c;
      sig_wb_wren_c = 24'(hpc_c) && op_c != RV32I::OP_STORE && op_c != RV32I::OP_BRANCH;

      if (5'(temp_wb_addr_c) == 0) sig_wb_wren_c = 0;

      if ((op_b == RV32I::OP_LOAD) && regfile_cs_b && (24'(hpc_a) == 0)) begin
        reg_raddr1_a = 10'(sig_addr_b >> 2);
      end

      // Handle stores through the bus to the regfile.
      if (op_c == RV32I::OP_STORE && regfile_cs_c) begin
        temp_wb_addr_c = 10'(addr_c >> 2);
        sig_wb_data_c = temp_result_c;
        sig_wb_wren_c = 1;
      end
      regs_tick_raddr1 = reg_raddr1_a;
      regs_tick_raddr2 = reg_raddr2_a;
      regs_tick_waddr = temp_wb_addr_c;
      regs_tick_wdata = sig_wb_data_c;
      regs_tick_wren = sig_wb_wren_c;

      sig_wb_addr_c = temp_wb_addr_c;
    end

    sig_result_c = temp_result_c;
  end

  //----------------------------------------

  always_ff @(posedge clock) begin : tick

    if (tick_reset_in) begin
      hpc_a     <= 32'h00400000;

      hpc_b     <= 0;
      insn_b    <= 0;

      hpc_c     <= 0;
      insn_c    <= 0;
      addr_c    <= 0;
      result_c  <= 0;

      hpc_d     <= 0;
      insn_d    <= 0;
      result_d  <= 0;
      wb_addr_d <= 0;
      wb_data_d <= 0;
      wb_wren_d <= 0;

      ticks     <= 0;
    end
    else begin
      hpc_d     <= hpc_c;
      insn_d    <= insn_c;
      result_d  <= sig_result_c;
      wb_addr_d <= sig_wb_addr_c;
      wb_data_d <= sig_wb_data_c;
      wb_wren_d <= sig_wb_wren_c;

      hpc_c     <= hpc_b;
      insn_c    <= insn_b;
      addr_c    <= sig_addr_b;
      result_c  <= sig_result_b;

      hpc_b     <= hpc_a;
      insn_b    <= sig_insn_a;

      hpc_a     <= sig_next_hpc_a;

      ticks     <= ticks + 1;
    end
  end

  //----------------------------------------
  // Signals to code ram

  /* verilator lint_off UNUSEDSIGNAL */
  /* verilator lint_on UNUSEDSIGNAL */

  //----------------------------------------
  // Signals to data bus

  /* verilator lint_off UNUSEDSIGNAL */
  /* verilator lint_on UNUSEDSIGNAL */

  //----------------------------------------
  // metron_internal

  logic[31:0] sig_insn_a;     // Signal
  logic[31:0] sig_next_hpc_a; // Signal

  logic[31:0] sig_addr_b;     // Signal
  logic[31:0] sig_result_b;   // Signal

  logic[31:0] sig_result_c;   // Signal
  logic[9:0] sig_wb_addr_c;  // Signal
  logic[31:0] sig_wb_data_c;  // Signal
  logic  sig_wb_wren_c;  // Signal

  //----------------------------------------
  // Registers

  regfile   regs(
    // Global clock
    .clock(clock),
    // tick() ports
    .tick_raddr1(regs_tick_raddr1),
    .tick_raddr2(regs_tick_raddr2),
    .tick_waddr(regs_tick_waddr),
    .tick_wdata(regs_tick_wdata),
    .tick_wren(regs_tick_wren),
    // get_rs1() ports
    .get_rs1_ret(regs_get_rs1_ret),
    // get_rs2() ports
    .get_rs2_ret(regs_get_rs2_ret)
  );
  logic[9:0] regs_tick_raddr1;
  logic[9:0] regs_tick_raddr2;
  logic[9:0] regs_tick_waddr;
  logic[31:0] regs_tick_wdata;
  logic regs_tick_wren;
  logic[31:0] regs_get_rs1_ret;
  logic[31:0] regs_get_rs2_ret;

  logic[31:0] ticks;

  logic[31:0] hpc_a;

  logic[31:0] hpc_b;
  logic[31:0] insn_b;

  logic[31:0] hpc_c;
  logic[31:0] insn_c;
  logic[31:0] addr_c;
  logic[31:0] result_c;

  /* verilator lint_off UNUSEDSIGNAL */
  logic[31:0] hpc_d;
  logic[31:0] insn_d;
  logic[31:0] result_d;
  logic[9:0] wb_addr_d;
  logic[31:0] wb_data_d;
  logic  wb_wren_d;
  /* verilator lint_on UNUSEDSIGNAL */

  //----------------------------------------
  // FIXME support static

  /* verilator lint_off UNUSEDSIGNAL */
  function logic[31:0] decode_imm(logic[31:0] insn);
    logic[4:0]  op;
    logic[31:0] imm_i;
    logic[31:0] imm_s;
    logic[31:0] imm_u;
    logic[31:0] imm_b;
    logic[31:0] imm_j;
    logic[31:0] result;
    op    = insn[6:2];
    imm_i = $signed(insn[31:20]);
    imm_s = {{21 {insn[31]}}, insn[30:25], insn[11:7]};
    imm_u = insn[31:12] << 12;
    imm_b = {{20 {insn[31]}}, insn[7], insn[30:25], insn[11:8], 1'd0};
    imm_j = {{12 {insn[31]}}, insn[19:12], insn[20], insn[30:21], 1'd0};

    case(op)
      RV32I::OP_LOAD:   result = imm_i;
      RV32I::OP_OPIMM:  result = imm_i;
      RV32I::OP_AUIPC:  result = imm_u;
      RV32I::OP_STORE:  result = imm_s;
      RV32I::OP_OP:     result = imm_i;
      RV32I::OP_LUI:    result = imm_u;
      RV32I::OP_BRANCH: result = imm_b;
      RV32I::OP_JALR:   result = imm_i;
      RV32I::OP_JAL:    result = imm_j;
      default:              result = 0;
    endcase
    decode_imm = result;
  endfunction
  /* verilator lint_on UNUSEDSIGNAL */

  //----------------------------------------

  function logic[31:0] execute_alu(logic[31:0] insn, logic[31:0] reg_a, logic[31:0] reg_b);
    logic[4:0]  op;
    logic[2:0]  f3;
    logic[6:0]  f7;
    logic[31:0] imm;
    logic[31:0] alu_a;
    logic[31:0] alu_b;
    logic[31:0] result;
    op  = insn[6:2];
    f3  = insn[14:12];
    f7  = insn[31:25];
    imm = decode_imm(insn);

    alu_a = reg_a;
    alu_b = op == RV32I::OP_OPIMM ? imm : reg_b;
    if (op == RV32I::OP_OP && f3 == 0 && f7 == 32) alu_b = -alu_b;

    case (f3)
      0:  result = alu_a + alu_b;
      1:  result = alu_a << 5'(alu_b);
      2:  result = $signed(alu_a) < $signed(alu_b);
      3:  result = alu_a < alu_b;
      4:  result = alu_a ^ alu_b;
      5:  result = f7 == 32 ? $signed(alu_a) >> 5'(alu_b) : alu_a >> 5'(alu_b);
      6:  result = alu_a | alu_b;
      7:  result = alu_a & alu_b;
      default: result = 0;
    endcase
    execute_alu = result;
  endfunction

  //----------------------------------------

  /* verilator lint_off UNUSEDSIGNAL */
  function logic[31:0] execute_system(logic[31:0] insn, logic[31:0] reg_a, logic[31:0] reg_b);
    logic[2:0]  f3;
    logic[11:0] csr;
    logic[31:0] result;
    f3  = insn[14:12];
    csr = insn[31:20];

    // FIXME need a good error if case is missing an expression
    result = 0;
    case(f3)
      RV32I::F3_CSRRW: begin
        result = reg_a;
      end
      RV32I::F3_CSRRS: begin
        if (csr == 12'hF14) result = hpc_b[31:24];
      end
      RV32I::F3_CSRRC:  result = 0;
      RV32I::F3_CSRRWI: result = 0;
      RV32I::F3_CSRRSI: result = 0;
      RV32I::F3_CSRRCI: result = 0;
    endcase
    execute_system = result;
  endfunction
  /* verilator lint_on UNUSEDSIGNAL */

endmodule
