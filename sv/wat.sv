module bot();
  parameter filename = "";

  initial begin
    if (filename) begin
      $readmemh(filename, buffer);
    end
  end

  logic[7:0] buffer[0:1023];
endmodule

module top();
  bot #(.filename("")) _bot();
endmodule
