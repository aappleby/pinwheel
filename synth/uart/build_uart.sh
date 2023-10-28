set -e
yosys -p 'read_verilog -I. -sv uart_test_ice40.sv; dump; synth_ice40 -json out/uart_test_ice40.json;'
nextpnr-ice40 -q --hx8k --package ct256 --json out/uart_test_ice40.json --asc out/uart_test_ice40.asc --pcf uart_hx8k.pcf
icepack out/uart_test_ice40.asc out/uart_test_ice40.bin
iceprog -S out/uart_test_ice40.bin
