MEMORY {
  ROM (rx) : ORIGIN = 0x00000000, LENGTH = 64K
  RAM (rw) : ORIGIN = 0x80000000, LENGTH = 64K
}

_stack_top = ORIGIN(RAM) + LENGTH(RAM);
PROVIDE( __global_pointer$ = 0x800 );

SECTIONS {
  .text : {
    _stext = .;
    *(.start);
    *(.text);
    *(.text.*);
    _etext = .;
  } >ROM

  .data : {
    _sdata = .;
    *(.RAM);
    *(.data);
    *(.data.*);
    *(.sdata);
    *(.sdata.*);
    *(.rodata);
    *(.rodata.*);
    _edata = .;
    _sbss = .;
    *(.bss);
    _ebss = .;
  } >RAM
}
