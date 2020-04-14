#paths 
RTL_DIR := ../../rtl
INC_DIR := $(RTL_DIR)/include
SRC := $(RTL_DIR)/src/*.v
INC := $(INC_DIR)/*.vh


#icarus simulator and flags
VC := iverilog
CFLAGS := -W all -I$(INC_DIR)  -g2005-sv


# select testbench file
ifeq ($(CMDGOALS),)
CMDGOALS := xversat
endif

TB := $(RTL_DIR)/testbench/$(CMDGOALS)_tb.v

#
#  VERSAT SIMULATION
#

all: $(SRC) $(INC) $(TB)
	$(VC) $(CFLAGS) $(SRC) $(TB)
	./a.out
#	diff -q data_out.hex expected.hex

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

# CLEANUP, ETC
clean:
	@rm -f *~ *# *.vcd a.out

.PHONY: all clean
