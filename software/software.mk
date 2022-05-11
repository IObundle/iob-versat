include $(VERSAT_DIR)/core.mk

#include
INCLUDE+=-I$(VERSAT_SW_DIR)

#headers
VERSAT_HDR += $(wildcard $(VERSAT_SW_DIR)/*.hpp)

#Units to verilate
VERILATE_UNIT = xadd xreg xmem vread vwrite

UNIT_HDR+=$(foreach unit,$(VERILATE_UNIT),./build/V$(unit).h)

OBJ+=./build/verilated.o
OBJ+=./build/verilated_vcd_c.o

./build/verilated.o: $(VERSAT_HDR)
	mkdir -p ./build;
	g++ -std=c++11 -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -m32 -g -c $(VERILATOR_INCLUDE)/verilated.cpp
	mv *.o ./build/;
	mv *.d ./build/;

./build/verilated_vcd_c.o: $(VERSAT_HDR)
	mkdir -p ./build;
	g++ -std=c++11 -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -m32 -g -c $(VERILATOR_INCLUDE)/verilated_vcd_c.cpp
	mv *.o ./build/;
	mv *.d ./build/;

./build/V%.h: $(VERSAT_HW_DIR)/src/%.v $(VERSAT_HDR)
	verilator --trace -CFLAGS "-g -m32 -std=c++11" -I$(VERSAT_HW_DIR)/src -I$(VERSAT_HW_DIR)/include -I$(MEM_DIR)/ram/2p_ram -I$(CACHE_DIR)/submodules/MEM/tdp_ram -cc -Mdir ./obj $<;
	cd ./obj && make -f V$*.mk;
	mkdir -p ./build; mv ./obj/*.o ./build;
	mv ./obj/*.h ./build
	rm -r -f ./obj

PHONY: clean
