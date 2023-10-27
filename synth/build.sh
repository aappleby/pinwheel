set -e
yosys -p 'read_verilog -I. -sv top_ice40.sv; dump; synth_ice40 -json top_ice40.json;'
nextpnr-ice40 -q --hx8k --package ct256 --json top_ice40.json --asc top_ice40.asc --pcf ice40/iCE40HX8K-B-EVN.pcf
icepack top_ice40.asc top_ice40.bin
iceprog -S top_ice40.bin
