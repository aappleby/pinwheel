
.section .start
_start:
.global _start
.option push
.option norelax

  la gp, __global_pointer$
  la sp, _stack_top
  csrr t0, mhartid
  /* 1024 bytes stack per hart*/
  sll t0, t0, 10
  sub sp, sp, t0
  j main

.option pop
