include $(VERSAT_DIR)/core.mk

#include
INCLUDE+=-I$(VERSAT_SW_DIR)

#headers
HDR+=$(VERSAT_SW_DIR)/*.h

#sources
#SRC+=$(VERSAT_SW_DIR)/iob-uart.c

#Units to verilate
VERILATE_UNIT += xadd
VERILATE_UNIT += xreg
VERILATE_UNIT += xmem
VERILATE_UNIT += vread
VERILATE_UNIT += vwrite

OBJ += $(foreach unit,$(VERILATE_UNIT),./build/$(unit).o)
OBJ += ./build/verilated.o

INCLUDE_VERILATOR = -I$(VERSAT_HW_DIR)/src -I$(VERSAT_HW_DIR)/include -I$(MEM_DIR)/ram/2p_ram -I$(CACHE_DIR)/submodules/MEM/tdp_ram

./build/verilated.o:
	mkdir -p ./build;
	g++  -I.  -MMD -I/usr/local/share/verilator/include -I/usr/local/share/verilator/include/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -c -o ./build/verilated.o /usr/local/share/verilator/include/verilated.cpp

./build/%.o: $(VERSAT_HW_DIR)/src/%.v
	verilator $(INCLUDE_VERILATOR) -cc -Mdir ./obj $<;
	cd ./obj && make -f V$*.mk;
	mkdir -p ./build; mv ./obj/V$*__ALL.o ./build/$*.o;
	mv ./obj/V$*.h ./build/$*.h
	rm -r -f ./obj

PHONY: clean