
TOOLCHAIN_PREFIX = /opt/riscv32i/bin/riscv32-unknown-elf-
GCC = $(TOOLCHAIN_PREFIX)gcc
GCC_WARNS = -Werror -Wall -Wno-unused-label
GCC_OPTIONS = -march=rv32i -Os -ffreestanding -nostdlib
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy

bin/test_firmware.bin: bin/test_firmware.elf
	$(OBJCOPY) -O binary bin/test_firmware.elf bin/test_firmware.bin

bin/rv32i_test.blif: rv32i.v rv32i_test.v blockram.v
	@echo "========== Synth $< =========="
	yosys -p "synth_ice40 -blif bin/rv32i_test.blif" rv32i_test.v

bin/rv32i_test.arachne: rv32i_test.pcf bin/rv32i_test.blif
	@echo "========== Place & route =========="
	arachne-pnr -d 8k -p rv32i_test.pcf bin/rv32i_test.blif -o bin/rv32i_test.arachne

bin/test_firmware.elf: boot.s test_firmware.c test_firmware.lds
	@echo "========== Building firmware =========="
	$(GCC) -c $(GCC_OPTIONS) -o bin/boot.o boot.s
	$(GCC) -c -S $(GCC_OPTIONS) $(GCC_WARNS) test_firmware.c -o bin/test_firmware.asm
	$(GCC) -c $(GCC_OPTIONS) $(GCC_WARNS) test_firmware.c -o bin/test_firmware.o
	$(GCC) $(GCC_OPTIONS) -o bin/test_firmware.elf \
	  -Wl,-Bstatic,-T,test_firmware.lds,-Map,bin/test_firmware.map,--strip-debug \
	  bin/boot.o bin/test_firmware.o

bin/test_firmware.hex: bin/test_firmware.bin
	python3 makehex.py bin/test_firmware.bin 2048 > bin/test_firmware.hex

bin/rv32i_test.arachne_prog: bin/test_firmware.hex bin/rv32i_test.arachne
	@echo "========== Replacing firmware =========="
	icebram test_firmware_placeholder.hex bin/test_firmware.hex < bin/rv32i_test.arachne > bin/rv32i_test.arachne_prog

bin/rv32i_test.bitfile: bin/rv32i_test.arachne_prog
	@echo "========== Packing =========="
	icepack bin/rv32i_test.arachne_prog bin/rv32i_test.bitfile

flash_rv32i_test: bin/rv32i_test.bitfile
	@echo "========== Programming =========="
	iceprog -S bin/rv32i_test.bitfile

bin/rv32i_bench.vvp: rv32i.v rv32i_test.v rv32i_bench.v bin/test_firmware.hex icarus_firmware.hex
	iverilog -Wall -Wno-timescale -g2012 -gassertions -o bin/rv32i_bench.vvp rv32i_bench.v

sim_rv32i: bin/rv32i_bench.vvp
	vvp -N bin/rv32i_bench.vvp

clean:
	rm -f bin/*

.PHONY: clean sim_rv32i
