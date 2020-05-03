#
# SYNTHESIS AND IMPLEMENTATION SCRIPT
#

#include files
read_verilog ../../../rtl/include/xaludefs.vh
read_verilog ../../../rtl/include/xalulitedefs.vh
read_verilog ../../../rtl/include/xbsdefs.vh
read_verilog ../../../rtl/include/xconfdefs.vh
read_verilog ../../../rtl/include/xmemdefs.vh
read_verilog ../../../rtl/include/xmuladddefs.vh
read_verilog ../../../rtl/include/xmuldefs.vh
read_verilog ../../../rtl/include/xdefs.vh
read_verilog ../../../rtl/include/versat/xversat.vh

#source files
read_verilog ../../../rtl/src/xaddrgen.v
read_verilog ../../../rtl/src/xaddrgen2.v
read_verilog ../../../rtl/src/xalu.v
read_verilog ../../../rtl/src/xalulite.v
read_verilog ../../../rtl/src/xbs.v
read_verilog ../../../rtl/src/xclz.v
read_verilog ../../../rtl/src/xconf.v
read_verilog ../../../rtl/src/xconf_mem.v
read_verilog ../../../rtl/src/xconf_reg.v
read_verilog ../../../rtl/src/xdata_eng.v
read_verilog ../../../rtl/src/xinmux.v
read_verilog ../../../rtl/src/xmem.v
read_verilog ../../../rtl/src/xmul.v
read_verilog ../../../rtl/src/xmul_pipe.v
read_verilog ../../../rtl/src/xmuladd.v
read_verilog ../../../rtl/src/xstage.v
read_verilog ../../../rtl/src/xversat.v
read_verilog ../../../submodules/mem/tdp_mem/iob_tdp_mem.v
read_verilog ../../../submodules/mem/sp_mem/iob_sp_mem.v
set_property file_type SystemVerilog [get_files xaddrgen.v]
set_property file_type SystemVerilog [get_files xaddrgen2.v]
set_property file_type SystemVerilog [get_files xalu.v]
set_property file_type SystemVerilog [get_files xalulite.v]
set_property file_type SystemVerilog [get_files xbs.v]
set_property file_type SystemVerilog [get_files xclz.v]
set_property file_type SystemVerilog [get_files xconf.v]
set_property file_type SystemVerilog [get_files xconf_mem.v]
set_property file_type SystemVerilog [get_files xconf_reg.v]
set_property file_type SystemVerilog [get_files xdata_eng.v]
set_property file_type SystemVerilog [get_files xinmux.v]
set_property file_type SystemVerilog [get_files xmem.v]
set_property file_type SystemVerilog [get_files xmul.v]
set_property file_type SystemVerilog [get_files xmul_pipe.v]
set_property file_type SystemVerilog [get_files xmuladd.v]
set_property file_type SystemVerilog [get_files xstage.v]
set_property file_type SystemVerilog [get_files xversat.v]

#Constraints file
read_xdc ./xversat.xdc

#Run synthesis
synth_design -part xcku040-fbva676-1-c -top xversat

#Run implementation
opt_design
place_design
route_design

#Run report
report_utilization
report_timing
