.section .text
.global main

/*
x1      = ra     = return address
x2      = fp     = frame pointer
x3-x13  = s1-s11 = saved registers
x14     = sp     = stack pointer
x15     = tp     = thread pointer
x16-17  = v0-v1  = return values
x18-x25 = a0-a7  = function arguments
x26-x30 = t0-t4  = temporaries
x31     = gp     = global pointer
*/

/*----------------------------------------------------------------------------*/

reset:
// Get hart ID
csrr t0, 0xF10

// Use it to set up stack pointer
li  t1, 8192
shl t2, t0, 8
sub sp, t1, t2

// Jump to main, passing hart in arg 3
li a0, 0
li a1, 0
li a2, t0
j main
nop
