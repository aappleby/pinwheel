rm -f ~/.yosys_show.*
#yosys -Q -p 'logger -werror .; read_verilog -sv bram.sv; hierarchy -top pinwheel_mem; synth_ice40; stat; show;'
yosys -Q -p 'logger -werror .; read_verilog -sv bram.sv; hierarchy -top pinwheel_regs; synth_ice40; stat; show;'
