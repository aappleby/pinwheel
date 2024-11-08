rm -f hx8k_top.json
rm -f hx8k_top.asc
rm -f hx8k_top.bin
set -v
yosys -Q -p 'logger -werror .; read_verilog -sv hx8k_top.sv; synth_ice40 -json hx8k_top.json;'
nextpnr-ice40 -q --hx8k --package ct256 --json hx8k_top.json --asc hx8k_top.asc --pcf hx8k_top.pcf
icepack hx8k_top.asc hx8k_top.bin
iceprog -S hx8k_top.bin
