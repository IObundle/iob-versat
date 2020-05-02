#source paths
MEM_DIR := ../../submodules/mem
RTL_DIR = ../../rtl

#include paths
INCLUDE_DIR := $(RTL_DIR)/include
HW_INCLUDE = -incdir $(INCLUDE_DIR) -incdir $(INCLUDE_DIR)/versat

#hardware defines
HW_DEFINE = -define DEBUG

#simulator flags
CFLAGS = -errormax 15 -status -update -linedebug -sv $(HW_DEFINE) $(HW_INCLUDE)
EFLAGS = -errormax 15 -access +wc -status
SFLAGS = -errormax 15 -status -CMDFILE $(RTL_DIR)/testbench

#select testbench file
ifeq ($(CMDGOALS),)
CMDGOALS := xversat
endif
TB_DIR := $(RTL_DIR)/testbench
TB := $(TB_DIR)/$(CMDGOALS)_tb.v

#hardware sources
VSRC := $(RTL_DIR)/src/*.v $(MEM_DIR)/tdp_mem/*.v $(MEM_DIR)/sp_mem/*.v $(TB)

#
#  VERSAT MAIN SIMULATION
#

all:
	VSRC="$(VSRC)" CFLAGS="$(CFLAGS)" EFLAGS="$(EFLAGS)" SFLAGS="$(SFLAGS)" CMDGOALS="$(CMDGOALS)" ./run_ncsim.sh

#
#  SUBMODULE TEST
#

#source files of each submodule test
xaddrgen_src := $(RTL_DIR)/src/xaddrgen.v
xaddrgen2_src := $(xaddrgen_src) $(RTL_DIR)/src/xaddrgen2.v
xalu_src := $(RTL_DIR)/src/xclz.v $(RTL_DIR)/src/xalu.v
xconf_reg_src := $(RTL_DIR)/src/xconf_reg.v
xconf_src := $(xconf_reg_src) $(RTL_DIR)/src/xconf_mem.v $(RTL_DIR)/src/xconf.v $(MEM_DIR)/sp_mem/*.v
xmem_src := $(RTL_DIR)/src/xinmux.v $(xaddrgen2_src) $(MEM_DIR)/tdp_mem/*.v $(RTL_DIR)/src/xmem.v
xmul_pipe_src := $(RTL_DIR)/src/xmul_pipe.v
xdata_eng_src := $(xmem_src) $(xalu_src) $(xmul_pipe_src) $(RTL_DIR)/src/xalulite.v $(RTL_DIR)/src/xmul.v $(RTL_DIR)/src/xmuladd.v $(RTL_DIR)/src/xbs.v $(RTL_DIR)/src/xdata_eng.v

xaddrgen:
	VSRC="$($@_src) $(TB_DIR)/$@_tb.v" CFLAGS="$(CFLAGS)" EFLAGS="$(EFLAGS)" SFLAGS="$(SFLAGS)" CMDGOALS="$@" ./run_ncsim.sh

xaddrgen2:
	VSRC="$($@_src) $(TB_DIR)/$@_tb.v" CFLAGS="$(CFLAGS)" EFLAGS="$(EFLAGS)" SFLAGS="$(SFLAGS)" CMDGOALS="$@" ./run_ncsim.sh

xalu:
	VSRC="$($@_src) $(RTL_DIR)/src/xinmux.v $(TB_DIR)/$@_tb.v" CFLAGS="$(CFLAGS)" EFLAGS="$(EFLAGS)" SFLAGS="$(SFLAGS)" CMDGOALS="$@" ./run_ncsim.sh

xconf_reg:
	VSRC="$($@_src) $(TB_DIR)/$@_tb.v" CFLAGS="$(CFLAGS)" EFLAGS="$(EFLAGS)" SFLAGS="$(SFLAGS)" CMDGOALS="$@" ./run_ncsim.sh

xconf:
	VSRC="$($@_src) $(TB_DIR)/$@_tb.v" CFLAGS="$(CFLAGS)" EFLAGS="$(EFLAGS)" SFLAGS="$(SFLAGS)" CMDGOALS="$@" ./run_ncsim.sh

xmem:
	VSRC="$($@_src) $(TB_DIR)/$@_tb.v" CFLAGS="$(CFLAGS)" EFLAGS="$(EFLAGS)" SFLAGS="$(SFLAGS)" CMDGOALS="$@" ./run_ncsim.sh

xmul_pipe:
	VSRC="$($@_src) $(TB_DIR)/$@_tb.v" CFLAGS="$(CFLAGS)" EFLAGS="$(EFLAGS)" SFLAGS="$(SFLAGS)" CMDGOALS="$@" ./run_ncsim.sh

xdata_eng:
	VSRC="$($@_src) $(TB_DIR)/$@_tb.v" CFLAGS="$(CFLAGS)" EFLAGS="$(EFLAGS)" SFLAGS="$(SFLAGS)" CMDGOALS="$@" ./run_ncsim.sh

clean:
	@rm -f *# *~ *.vcd *.dat *.hex firmware.bin xmsim.key *.log *.txt
	@rm -rf xcelium.d

.PHONY: all clean
