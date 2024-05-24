set -v
rm -f testbench
iverilog -g2012 testbench.sv -s testbench -o testbench

./testbench
