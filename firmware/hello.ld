MEMORY {
  ROM (rxai!w) : ORIGIN = 0x00400000, LENGTH = 64K
  RAM (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128K
}

_stack_top = ORIGIN(RAM) + LENGTH(RAM);

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
    PROVIDE( __global_pointer$ = . + 0x800 );
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
