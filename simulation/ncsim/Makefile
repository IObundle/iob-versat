RTL_DIR = ../../rtl
VA_DIR = ../../tools/assembler
TESTS_DIR = ../../tests
INCLUDE_DIR := $(RTL_DIR)/include

CFLAGS = -errormax 15 -incdir $(INCLUDE_DIR) -status -update -linedebug -sv
EFLAGS = -errormax 15 -access +wc -status
SFLAGS = -errormax 15 -status

VHEADERS := $(shell find $(INCLUDE_DIR) -name '*.vh')
SRC := $(shell find $(RTL_DIR)/src -name '*.v')

ifeq ($(CMDGOALS),)
CMDGOALS := xtop
endif

# testbench
TB := $(RTL_DIR)/tb/$(CMDGOALS)_tb.v

# dictionary of symbols
DICT := $(RTL_DIR)/tb/xdict.v

#default test
test := src_fe


#
#  VERSAT MAIN SIMULATION
#
#  (in future can load sw w/o recompiling)
#
xtop: $(SRC) $(HEADERS) $(TB) versat_sw
	ncvlog $(CFLAGS) $(SRC) $(TB) -define DMA_IF -define ALTERA -define DEBUG $(hwdefines)
	ncelab $(EFLAGS) worklib.xtop_tb:module
	ncsim  $(SFLAGS) worklib.xtop_tb:module
	diff -q ./data_out.hex expected.hex


#
#  VERSAT SYNTHESIZED SIMULATION
#

xtop_synth: $@.v $@_tb.v versat_sw \
../asic_memories/SEHD130_64X32X1CM4_beh.v \
../asic_memories/SJHD130_2048X32X1CM4_beh.v
	ncvlog $(CFLAGS) $(SRC) ../synth/xtop_synth.v /opt/ic_tools/pdk/faraday/umc130/HS/fsc0h_d/2009Q1v3.0/GENERIC_CORE/FrontEnd/verilog/fsc0h_d_generic_core_30.lib -define SYNTH
	ncelab $(EFLAGS) worklib.xtop_tb:module
	ncsim +neg_tchk $(SFLAGS) worklib.xtop__tb:module -input commands.txt
	diff -q ./data_out.hex expected.hex

#
#  VERSAT SOFTWARE
#

versat_sw: FORCE xdict.json
	make -C $(TESTS_DIR)/$(test) $(testparams)
	cp $(TESTS_DIR)/$(test)/bootrom.hex .
	cp $(TESTS_DIR)/$(test)/data_in.hex .
#mp3_fe is tested using shine-hwtest
ifneq ($(test), mp3_fe)
	cp $(TESTS_DIR)/$(test)/expected.hex .
endif

#
#  VERSAT DICTIONARY
#

xdict.json: $(DICT) $(VHEADERS)
	ncvlog $(CFLAGS) $(DICT)
	ncelab $(EFLAGS) worklib.xdict:module
	ncsim  $(SFLAGS) worklib.xdict:module
	cp xdict.json $(TESTS_DIR)/$(test)/

# CLEANUP, ETC
clean:
	make -C $(TESTS_DIR)/$(test) clean
	@rm -f *#
	@rm -f xtop.tcf
	@rm -f ncsim.key
	@rm -f *.hex
	@rm -f xdict.json
	@rm -f *.log
	@rm -f *~
	@rm -rf INCA_libs
	@rm -f *.vcd

FORCE:

.phony: versat_sim versat_sw  xtop_synth clean FORCE
