RTL_DIR := ../../rtl
VA_DIR := ../../tools/assembler
TESTS_DIR := ../../tests
INCLUDE_DIR := $(RTL_DIR)/include

VC := iverilog
CFLAGS := -W all -I$(INCLUDE_DIR)  -g2005-sv

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

all: $(SRC) $(VHEADERS) $(TB) versat_sw
	$(VC) $(CFLAGS) -D PAR_IF -D DMA_USE -D DMA_IF -D ALTERA -D DEBUG $(SRC) $(TB) -s $@_tb
	./a.out
	diff -q data_out.hex expected.hex

#
# SUBMODULE TEST
#
xaddrgen: $@.v $(TB) xmemdefs.vh
	$(VC) $(CFLAGS) $(hwdefines) $@.v $(TB)
	./a.out

xalu: $@.v $(TB) xinmux.v xclz.v xaludefs.vh
	$(VC) $(CFLAGS) $(hwdefines) $@.v $(TB)
	./a.out

xalulite: $@.v $(TB) xinmux.v xclz.v xalulitedefs.vh
	$(VC) $(CFLAGS) $(hwdefines) $@.v $(TB)
	./a.out

xconf_reg: $@.v $(TB)
	$(VC) $(CFLAGS) $(hwdefines) $@.v $(TB)
	./a.out

xconf_mem: $@.v $(TB) $(SRC) maketest
	$(VC) $(CFLAGS) -D PAR_IF -D DMA_IF -D ALTERA $(hwdefines) $(SRC)
	./a.out

xconf_mem_slice: $@.v $(TB)
	$(VC) $(CFLAGS) $(hwdefines) $@.v $(TB)
	./a.out

#
#  VERSAT SOFTWARE
#

versat_sw: FORCE xdict.json
	make -C $(TESTS_DIR)/$(test) $(testparams)
	cp $(TESTS_DIR)/$(test)/*.hex ./

#
#  DICTIONARY
#

xdict.json: $(DICT) $(VHEADERS)
	$(VC) $(CFLAGS) $(DICT)
	./a.out
	cp xdict.json $(TESTS_DIR)/$(test)/

# CLEANUP, ETC
clean:
	make -C $(TESTS_DIR)/$(test) clean
	@rm -f *#
	@rm -f xdict.json
	@rm -f *.hex
	@rm -f *~ 
	@rm -f *.vcd
	@rm -f a.out

FORCE: 

.phony: clean FORCE versat_sw
