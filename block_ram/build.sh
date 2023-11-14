set -v
iverilog -g2012 bram_testbench.sv -o bram_testbench

./bram_testbench
