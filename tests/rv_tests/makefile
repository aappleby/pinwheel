SRCS=$(wildcard *.S)
OBJS=$(SRCS:.S=.o)
ELFS=$(SRCS:.S=.elf)

all: ${ELFS}

%.o: %.S test_macros.h riscv_test.h
	riscv64-unknown-elf-gcc -c -mabi=ilp32 -march=rv32i -o $@ $<

%.elf: %.o
	riscv64-unknown-elf-ld -Tlink.ld -melf32lriscv $< -o $@

clean:
	rm -f ${OBJS} ${ELFS}

.PHONY: clean
.PRECIOUS: %.elf
