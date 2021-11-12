include $(VERSAT_DIR)/core.mk

#include
INCLUDE+=-I$(VERSAT_SW_DIR)

#headers
HDR+=$(VERSAT_SW_DIR)/*.h

#sources
#SRC+=$(VERSAT_SW_DIR)/iob-uart.c

#Units to verilate
VERILATE_UNIT += xadd

OBJ += $(foreach unit,$(VERILATE_UNIT),./build/$(unit).o)

INCLUDE_VERILATOR = -I$(VERSAT_HW_DIR)/src -I$(VERSAT_HW_DIR)/include -I$(MEM_DIR)/2p_ram -I$(CACHE_DIR)/submodules/MEM/tdp_ram

./build/%.o: $(VERSAT_HW_DIR)/src/%.v
	verilator $(INCLUDE_VERILATOR) -cc -Mdir ./obj $<;
	cd ./obj && make -f V$*.mk;
	mkdir -p ./build; mv ./obj/V$*__ALL.o ./build/$*.o;
	mv ./obj/V$*.h ./build/$*.h
	rm -r -f ./obj

PHONY: clean