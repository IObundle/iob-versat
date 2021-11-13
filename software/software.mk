include $(VERSAT_DIR)/core.mk

#include
INCLUDE+=-I$(VERSAT_SW_DIR)

#headers
HDR+=$(VERSAT_SW_DIR)/*.h

#sources
#SRC+=$(VERSAT_SW_DIR)/iob-uart.c

#Generate verilated.o

#Units to verilate
VERILATE_UNIT += xadd

OBJ += $(foreach unit,$(VERILATE_UNIT),$(VERSAT_SW_DIR)/build/$(unit).o)

INCLUDE_VERILATOR = -I$(VERSAT_HW_DIR)/src -I$(VERSAT_HW_DIR)/include -I$(MEM_DIR)/2p_ram -I$(CACHE_DIR)/submodules/MEM/tdp_ram

$(VERSAT_SW_DIR)/build/%.o: $(VERSAT_HW_DIR)/src/%.v
	verilator $(INCLUDE_VERILATOR) -cc -Mdir ./obj $<;
	cd ./obj && make -f V$*.mk;
	mkdir -p $(VERSAT_SW_DIR)/build; mv ./obj/V$*__ALL.o $(VERSAT_SW_DIR)/build/$*.o;
	mkdir -p $(VERSAT_SW_DIR)/headers; mv ./obj/V$*.h $(VERSAT_SW_DIR)/headers/$*.h
	rm -r -f ./obj
