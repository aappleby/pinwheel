`include "metron_tools.sv"

`include "block_ram.sv"
// metron_noconvert
/*#include "console.h"*/
`include "constants.sv"
`include "regfile.sv"

//------------------------------------------------------------------------------

// verilator lint_off unusedsignal
// verilator lint_off varhidden

// FIXME remove this once everything hooked up
// verilator lint_off UNDRIVEN

module pinwheel (
  // global clock
  input logic clock,
  // tock_twocycle() ports
  input logic tock_twocycle_reset_in,
  // get_debug() ports
  output logic[31:0] get_debug_ret,
  // tick_twocycle() ports
  input logic tick_twocycle_reset_in
);
/*public:*/

  // metron_noconvert
  /*void init(const char* text_file = nullptr, const char* data_file = nullptr) {
    readmemh(text_file, code.data);
    readmemh(data_file, data_ram.data);
  }*/

  // metron_noconvert
  /*pinwheel* clone() {
    pinwheel* p = new pinwheel();
    memcpy(p, this, sizeof(*this));
    return p;
  }*/

  // metron_noconvert
  /*size_t size_bytes() {
    return sizeof(*this);
  }*/

  // metron_noconvert
  /*bool load_elf(const char* firmware_filename);*/

  //----------

  /*
  void reset_mem() {
    memset(&code,    0x00, sizeof(code));
    memset(&data_ram,    0x00, sizeof(data_ram));
    memset(&regfile, 0,    sizeof(regfile));
  }
  */

  //----------------------------------------
  // FIXME support static

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

  //----------

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

  //----------

  function logic[31:0] execute_system(logic[31:0] insn);
    logic[2:0]  f3;
    logic[11:0] csr;
    logic[31:0] result;
    f3  = insn[14:12];
    csr = insn[31:20];

    // FIXME need a good error if case is missing an expression
    result = 0;
    case(f3)
      0:                result = 0;
      RV32I::F3_CSRRW:  result = 0;
      RV32I::F3_CSRRS:  if (csr == 12'hF14) result = hart_b;
      RV32I::F3_CSRRC:  result = 0;
      4:                result = 0;
      RV32I::F3_CSRRWI: result = 0;
      RV32I::F3_CSRRSI: result = 0;
      RV32I::F3_CSRRCI: result = 0;
    endcase
    execute_system = result;
  endfunction

  //----------------------------------------
  // FIXME const local variable should not become parameter

  always_comb begin : tock_twocycle
    logic[4:0] op_b;
    logic[4:0] rda_b;
    logic[2:0] f3_b;
    logic[4:0] rs1a_b;
    logic[4:0] rs2a_b;
    logic[6:0] f7_b;
    logic[4:0] op_c;
    logic[4:0] rd_c;
    logic[2:0] f3_c;
    logic[31:0] rs1_b;
    logic[31:0] rs2_b;
    logic[31:0] imm_b;
    logic[31:0] addr_b;
    logic[31:0] temp_pc_a;
    logic[31:0] insn_a;
    logic[4:0] rs1a_a;
    logic[4:0] rs2a_a;
    logic code_cs_b;
    logic console1_cs_b;
    logic console2_cs_b;
    logic console3_cs_b;
    logic console4_cs_b;
    logic data_cs_b;
    logic regfile_cs_b;
    logic debug_cs_b;
    logic[3:0]       temp_mask_b;
    logic[3:0]       temp_mask_c;
    logic code_cs_c;
    logic console1_cs_c;
    logic console2_cs_c;
    logic console3_cs_c;
    logic console4_cs_c;
    logic data_cs_c;
    logic debug_cs_c;
    logic regfile_cs_c;
    logic[31:0] data_out_c;
    logic[31:0]        unpacked_c;
    logic  code_wren_c;
    logic  data_wren_b;
    logic[31:0] code_addr_c;
    logic[31:0] data_addr_b;
    logic[9:0] reg_raddr1_a;
    logic[9:0] reg_raddr2_a;
    logic  regfile_wren_b;
    op_b   = insn_b[6:2];
    rda_b  = insn_b[11:7];
    f3_b   = insn_b[14:12];
    rs1a_b = insn_b[19:15];
    rs2a_b = insn_b[24:20];
    f7_b   = insn_b[31:25];

    op_c   = insn_c[6:2];
    rd_c   = insn_c[11:7];
    f3_c   = insn_c[14:12];

    rs1_b  = rs1a_b ? regs_get_rs1_ret : 32'd0;
    rs2_b  = rs2a_b ? regs_get_rs2_ret : 32'd0;
    imm_b  = decode_imm(insn_b);
    addr_b = 32'(rs1_b + imm_b);

    temp_pc_a = 0;

    next_hart_a    = hart_b;
    next_pc_a      = 0;
    next_insn_b    = 0;
    next_addr_c    = 0;
    next_result_c  = 0;
    next_wb_addr_d = 0;
    next_wb_data_d = 0;
    next_wb_wren_d = 0;
    next_debug_reg = 0;

    //----------
    // Fetch

    begin
      if (pc_b) begin
        logic eq;
        logic slt;
        logic ult;
        logic take_branch;
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

        case (op_b)
          RV32I::OP_BRANCH:  temp_pc_a = take_branch ? pc_b + imm_b : pc_b + 32'd4;
          RV32I::OP_JAL:     temp_pc_a = pc_b + imm_b;
          RV32I::OP_JALR:    temp_pc_a = addr_b;
          default:                temp_pc_a = pc_b + 4;
        endcase
      end
    end

    //----------
    // Decode

    insn_a = code_rdata_ret;
    rs1a_a  = insn_a[19:15];
    rs2a_a  = insn_a[24:20];

    next_insn_b = pc_a == 0 ? 32'd0 : insn_a;
    next_addr_c = addr_b;

    //----------
    // Execute

    case(op_b)
      RV32I::OP_JAL:     next_result_c = pc_b + 4;
      RV32I::OP_JALR:    next_result_c = pc_b + 4;
      RV32I::OP_LUI:     next_result_c = imm_b;
      RV32I::OP_AUIPC:   next_result_c = pc_b + imm_b;
      RV32I::OP_LOAD:    next_result_c = addr_b;
      RV32I::OP_STORE:   next_result_c = rs2_b;
      RV32I::OP_CUSTOM0: begin
        next_result_c = 0;
        if (f3_b == 0) begin
          // Switch the other thread to another hart
          next_addr_c   = rs1_b;
          next_result_c = rs2_b;
        end
        else if (f3_b == 1) begin
          // Yield to another hart
          next_result_c  = temp_pc_a;
          next_hart_a    = rs1_b;
          temp_pc_a      = rs2_b;
        end
      end
      RV32I::OP_SYSTEM:  next_result_c = execute_system(insn_b);
      default:                next_result_c = execute_alu   (insn_b, rs1_b, rs2_b);
    endcase

    //----------
    // Memory

    code_cs_b     = addr_b[31:28] == 4'h0;
    console1_cs_b = addr_b[31:28] == 4'h4;
    console2_cs_b = addr_b[31:28] == 4'h5;
    console3_cs_b = addr_b[31:28] == 4'h6;
    console4_cs_b = addr_b[31:28] == 4'h7;
    data_cs_b     = addr_b[31:28] == 4'h8;
    regfile_cs_b  = addr_b[31:28] == 4'hE;
    debug_cs_b    = addr_b[31:28] == 4'hF;

    temp_mask_b = 0;
    if (f3_b == 0) temp_mask_b = 4'b0001;
    if (f3_b == 1) temp_mask_b = 4'b0011;
    if (f3_b == 2) temp_mask_b = 4'b1111;
    if (addr_b[0]) temp_mask_b = temp_mask_b << 1;
    if (addr_b[1]) temp_mask_b = temp_mask_b << 2;

    temp_mask_c = 0;
    if (f3_c == 0) temp_mask_c = 4'b0001;
    if (f3_c == 1) temp_mask_c = 4'b0011;
    if (f3_c == 2) temp_mask_c = 4'b1111;
    if (addr_c[0]) temp_mask_c = temp_mask_c << 1;
    if (addr_c[1]) temp_mask_c = temp_mask_c << 2;

    next_debug_reg = (op_b == RV32I::OP_STORE) && debug_cs_b ? rs2_b : debug_reg;

    //----------
    // Write

    code_cs_c     = addr_c[31:28] == 4'h0 && temp_pc_a == 0;
    console1_cs_c = addr_c[31:28] == 4'h4;
    console2_cs_c = addr_c[31:28] == 4'h5;
    console3_cs_c = addr_c[31:28] == 4'h6;
    console4_cs_c = addr_c[31:28] == 4'h7;
    data_cs_c     = addr_c[31:28] == 4'h8;
    debug_cs_c    = addr_c[31:28] == 4'hF;
    regfile_cs_c  = addr_c[31:28] == 4'hE;

    data_out_c = 0;
    if (data_cs_c) begin
      data_out_c = data_ram_rdata_ret;
    end
    else if (debug_cs_c) begin
      data_out_c = debug_reg;
    end
    else if (regfile_cs_c) begin
      data_out_c = regs_get_rs1_ret;
    end

    unpacked_c = data_out_c;
    if (result_c[0]) unpacked_c = unpacked_c >> 8;
    if (result_c[1]) unpacked_c = unpacked_c >> 16;
    case (f3_c)
      0:  unpacked_c = $signed( 8'(unpacked_c));
      1:  unpacked_c = $signed(16'(unpacked_c));
      4:  unpacked_c = $unsigned( 8'(unpacked_c));
      5:  unpacked_c = $unsigned(16'(unpacked_c));
    endcase

    next_wb_addr_d = {5'(hart_c), rd_c};
    next_wb_data_d = op_c == RV32I::OP_LOAD ? unpacked_c : result_c;
    next_wb_wren_d = op_c != RV32I::OP_STORE && op_c != RV32I::OP_BRANCH;

    if (op_c == RV32I::OP_CUSTOM0 && f3_c == 0) begin
      // Swap result and the PC that we'll use to fetch.
      // Execute phase should've deposited the new PC in result
      next_wb_data_d = temp_pc_a;
      next_hart_a    = addr_c;
      temp_pc_a      = result_c;
    end

    if (regfile_cs_c && op_c == RV32I::OP_STORE) begin
      // Thread writing to other thread's regfile
      next_wb_addr_d = 10'(addr_c >> 2);
      next_wb_data_d = result_c;
      next_wb_wren_d = 1;
    end

    //----------
    // Code/data/reg read/write overrides for cross-thread stuff

    // Hmm we can't actually read from code because we also have to read our next instruction
    // and we can't do it earlier or later (we can read it during C, but then it's not back
    // in time to write to the regfile).

    code_wren_c = (op_c == RV32I::OP_STORE) && code_cs_c;
    data_wren_b = (op_b == RV32I::OP_STORE) && data_cs_b;

    code_addr_c = code_cs_c ? addr_c : temp_pc_a;
    data_addr_b = addr_b;

    reg_raddr1_a = {5'(hart_a), rs1a_a};
    reg_raddr2_a = {5'(hart_a), rs2a_a};
    regfile_wren_b = (op_b == RV32I::OP_STORE) && regfile_cs_b;

    if ((op_b == RV32I::OP_LOAD) && regfile_cs_b && (pc_a == 0)) begin
      reg_raddr1_a = 10'(addr_b >> 2);
    end

    //----------
    // Submod tocks
    code_tock_addr_ = 12'(code_addr_c);
    code_tock_wdata_ = result_c;
    code_tock_wmask_ = temp_mask_c;
    code_tock_wren_ = code_wren_c;

    data_ram_tock_addr_ = 12'(data_addr_b);
    data_ram_tock_wdata_ = rs2_b;
    data_ram_tock_wmask_ = temp_mask_b;
    data_ram_tock_wren_ = data_wren_b;
    regs_tock_raddr1_ = reg_raddr1_a;
    regs_tock_raddr2_ = reg_raddr2_a;
    regs_tock_waddr_ = next_wb_addr_d;
    regs_tock_wdata_ = next_wb_data_d;
    regs_tock_wren_ = next_wb_wren_d;

    // metron_noconvert
    /*{
      console1.tock(console1_cs_b && op_b == RV32I::OP_STORE, rs2_b);
      console2.tock(console2_cs_b && op_b == RV32I::OP_STORE, rs2_b);
      console3.tock(console3_cs_b && op_b == RV32I::OP_STORE, rs2_b);
      console4.tock(console4_cs_b && op_b == RV32I::OP_STORE, rs2_b);
    }*/

    //----------
    // Signal writeback

    next_pc_a = temp_pc_a;
  end

  //----------------------------------------

  always_comb begin : get_debug
    get_debug_ret = debug_reg;
  end

  //----------------------------------------
  // FIXME trace modules individually

  always_ff @(posedge clock) begin : tick_twocycle
    if (tick_twocycle_reset_in) begin
      //reset_mem();
      hart_a    <= 1;
      pc_a      <= 0;

      hart_b    <= 0;
      pc_b      <= 32'h00400000 - 4;
      insn_b    <= 0;

      hart_c    <= 0;
      pc_c      <= 0;
      insn_c    <= 0;
      addr_c    <= 0;
      result_c  <= 0;

      hart_d    <= 0;
      pc_d      <= 0;
      insn_d    <= 0;
      result_d  <= 0;
      wb_addr_d <= 0;
      wb_data_d <= 0;
      wb_wren_d <= 0;

      debug_reg <= 0;
      // metron_noconvert
      /*ticks     = 0;*/
    end
    else begin
      hart_d    <= hart_c;
      pc_d      <= pc_c;
      insn_d    <= insn_c;
      result_d  <= result_c;
      wb_addr_d <= next_wb_addr_d;
      wb_data_d <= next_wb_data_d;
      wb_wren_d <= next_wb_wren_d;

      hart_c    <= hart_b;
      pc_c      <= pc_b;
      insn_c    <= insn_b;
      addr_c    <= next_addr_c;
      result_c  <= next_result_c;

      hart_b    <= hart_a;
      pc_b      <= pc_a;
      insn_b    <= next_insn_b;

      hart_a    <= next_hart_a;
      pc_a      <= next_pc_a;

      debug_reg <= next_debug_reg;
      // metron_noconvert
      /*ticks     = ticks + 1;*/
    end


    // metron_noconvert
    /*console1.tick(reset_in);*/
    // metron_noconvert
    /*console2.tick(reset_in);*/
    // metron_noconvert
    /*console3.tick(reset_in);*/
    // metron_noconvert
    /*console4.tick(reset_in);*/
  end

  // metron_noconvert
  /*uint32_t* get_code() { return code.get_data(); }*/
  // metron_noconvert
  /*uint32_t* get_data() { return data_ram.get_data(); }*/

  //----------------------------------------

  // metron_internal
  logic[4:0]  next_hart_a;
  logic[31:0] next_pc_a;

  logic[31:0] next_insn_b;

  logic[31:0] next_addr_c;
  logic[31:0] next_result_c;

  logic[9:0] next_wb_addr_d;
  logic[31:0] next_wb_data_d;
  logic  next_wb_wren_d;

  logic[31:0] next_debug_reg;

  //----------

  logic[4:0]  hart_a;
  logic[31:0] pc_a;

  logic[4:0]  hart_b;
  logic[31:0] pc_b;
  logic[31:0] insn_b;

  logic[4:0]  hart_c;
  logic[31:0] pc_c;
  logic[31:0] insn_c;
  logic[31:0] addr_c;
  logic[31:0] result_c;

  logic[4:0]  hart_d;
  logic[31:0] pc_d;
  logic[31:0] insn_d;
  logic[31:0] result_d;
  logic[9:0] wb_addr_d;
  logic[31:0] wb_data_d;
  logic  wb_wren_d;

  logic[31:0] debug_reg;

  /*logic<32> gpio_dir;*/
  /*logic<32> gpio_in;*/
  /*logic<32> gpio_out;*/

  block_ram  code(
    // global clock
    .clock(clock),
    // tock() ports
    .tock_addr_(code_tock_addr_),
    .tock_wdata_(code_tock_wdata_),
    .tock_wmask_(code_tock_wmask_),
    .tock_wren_(code_tock_wren_),
    // rdata() ports
    .rdata_ret(code_rdata_ret)
  );
  logic[11:0] code_tock_addr_;
  logic[31:0] code_tock_wdata_;
  logic[3:0] code_tock_wmask_;
  logic code_tock_wren_;
  logic[31:0] code_rdata_ret;


  // FIXME having this named data and a field inside block_ram named data breaks context resolve
  block_ram  data_ram(
    // global clock
    .clock(clock),
    // tock() ports
    .tock_addr_(data_ram_tock_addr_),
    .tock_wdata_(data_ram_tock_wdata_),
    .tock_wmask_(data_ram_tock_wmask_),
    .tock_wren_(data_ram_tock_wren_),
    // rdata() ports
    .rdata_ret(data_ram_rdata_ret)
  );
  logic[11:0] data_ram_tock_addr_;
  logic[31:0] data_ram_tock_wdata_;
  logic[3:0] data_ram_tock_wmask_;
  logic data_ram_tock_wren_;
  logic[31:0] data_ram_rdata_ret;

  regfile   regs(
    // global clock
    .clock(clock),
    // tock() ports
    .tock_raddr1_(regs_tock_raddr1_),
    .tock_raddr2_(regs_tock_raddr2_),
    .tock_waddr_(regs_tock_waddr_),
    .tock_wdata_(regs_tock_wdata_),
    .tock_wren_(regs_tock_wren_),
    // get_rs1() ports
    .get_rs1_ret(regs_get_rs1_ret),
    // get_rs2() ports
    .get_rs2_ret(regs_get_rs2_ret)
  );
  logic[9:0] regs_tock_raddr1_;
  logic[9:0] regs_tock_raddr2_;
  logic[9:0] regs_tock_waddr_;
  logic[31:0] regs_tock_wdata_;
  logic regs_tock_wren_;
  logic[31:0] regs_get_rs1_ret;
  logic[31:0] regs_get_rs2_ret;


  // metron_noconvert
  /*Console console1;*/
  // metron_noconvert
  /*Console console2;*/
  // metron_noconvert
  /*Console console3;*/
  // metron_noconvert
  /*Console console4;*/

  // metron_noconvert
  /*uint64_t ticks;*/
endmodule

// verilator lint_on unusedsignal

//------------------------------------------------------------------------------
