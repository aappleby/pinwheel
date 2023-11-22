OUTPUT_ARCH("riscv")

ENTRY(_start)

MEMORY {
  ROM (xai!rw) : ORIGIN = 0x00000000, LENGTH = 4K
  RAM (wai!rx) : ORIGIN = 0x80000000, LENGTH = 4K
}

_stack_top = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
  .text : {
    *(.start);
    _stext = .;
    *(.text);
    *(.text.*);
    _etext = .;
    . = ALIGN(4);
  } >ROM

  .data : {
    _sdata = .;
    *(.RAM);
    *(.data);
    *(.data.*);
    PROVIDE( __global_pointer$ = . + 0x800 );
    *(.sdata);
    *(.sdata.*);
    *(.rodata);
    *(.rodata.*);
    _edata = .;
    _sbss = .;
    *(.bss);
    _ebss = .;
    . = ALIGN(4);
  } >RAM
}
