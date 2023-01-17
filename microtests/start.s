
.section .start
_start:
.global _start
.option push
.option norelax

  la gp, __global_pointer$
  la sp, _stack_top
  j main

.option pop
