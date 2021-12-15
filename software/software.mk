include $(VERSAT_DIR)/core.mk

#include
INCLUDE+=-I$(VERSAT_SW_DIR)

#headers
HDR+=$(VERSAT_SW_DIR)/*.h

#Units to verilate
VERILATE_UNIT = xadd xreg xmem vread vwrite

UNIT_OBJ+=$(foreach unit,$(VERILATE_UNIT),./build/V$(unit)__ALLcls.o)

OBJ+=./build/verilated.o
OBJ+=./build/verilated_vcd_c.o

./build/verilated.o:
	mkdir -p ./build;
	g++ -std=c++11 -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -m32 -g -c $(VERILATOR_INCLUDE)/verilated.cpp
	mv *.o ./build/;
	mv *.d ./build/;

./build/verilated_vcd_c.o:
	mkdir -p ./build;
	g++ -std=c++11 -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -m32 -g -c $(VERILATOR_INCLUDE)/verilated_vcd_c.cpp
	mv *.o ./build/;
	mv *.d ./build/;

./build/V%__ALLcls.o: $(VERSAT_HW_DIR)/src/%.v
	verilator --trace -CFLAGS "-g -m32 -std=c++11" -I$(VERSAT_HW_DIR)/src -I$(VERSAT_HW_DIR)/include -I$(MEM_DIR)/ram/2p_ram -I$(CACHE_DIR)/submodules/MEM/tdp_ram -cc -Mdir ./obj $<;
	cd ./obj && make -f V$*.mk;
	mkdir -p ./build; mv ./obj/*.o ./build;
	mv ./obj/*.h ./build
	rm -r -f ./obj

PHONY: clean
