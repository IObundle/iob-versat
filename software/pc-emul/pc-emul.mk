#versat common parameters
include $(VERSAT_DIR)/software/software.mk

#pc sources
HDR+=$(wildcard $(VERSAT_PC_EMUL)/*.hpp)

INCLUDE+= -I$(VERSAT_PC_EMUL)
DEFINE+= -DPC

CPP_FILES = $(wildcard $(VERSAT_PC_EMUL)/*.cpp)
CPP_OBJ = $(patsubst $(VERSAT_PC_EMUL)/%.cpp,./build/%.o,$(CPP_FILES))

#Units to verilate
VERILATE_UNIT = xadd xreg xmem vread vwrite pipeline_register
UNIT_VERILOG += $(foreach unit,$(VERILATE_UNIT),$(VERSAT_DIR)/hardware/src/$(unit).v)

UNIT_HDR+=$(foreach unit,$(VERILATE_UNIT),./build/V$(unit).h)

TYPE_INFO_HDR = $(VERSAT_PC_EMUL)/versat.hpp $(VERSAT_SW_DIR)/utils.hpp $(VERSAT_PC_EMUL)/verilogParser.hpp $(VERSAT_PC_EMUL)/templateEngine.hpp

TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/parser.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/utils.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/memory.cpp
TOOL_SRC += $(TOOL_COMMON_SRC)
TOOL_SRC += $(VERSAT_DIR)/software/pc-emul/templateEngine.cpp
TOOL_SRC += $(VERSAT_DIR)/software/pc-emul/type.cpp

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

./build/V%.h: $(VERSAT_HW_DIR)/src/%.v
	verilator --trace -CFLAGS "-g -m32 -std=c++11" -I$(VERSAT_HW_DIR)/src -I$(VERSAT_HW_DIR)/include -I$(VERSAT_DIR)/submodules/MEM/ram/2p_ram -I$(VERSAT_DIR)/submodules/MEM/ram/tdp_ram -cc -Mdir ./obj $<;
	cd ./obj && make -f V$*.mk;
	mkdir -p ./build; mv ./obj/*.o ./build;
	mv ./obj/*.h ./build
	rm -r -f ./obj

./build/typeInfo.inc: ./build/structParser.out $(TYPE_INFO_HDR)
	mkdir -p ./build
	./build/structParser.out ./build/typeInfo.inc $(TYPE_INFO_HDR)

./build/verilogWrapper.inc: ./build/verilogParser.out  $(VERSAT_SW_DIR)/pc-emul/verilogParser.cpp
	./build/verilogParser.out ./build/verilogWrapper.inc -I $(VERSAT_DIR)/submodules/INTERCON/hardware/include/ -I $(VERSAT_DIR)/hardware/include/ -I $(VERSAT_DIR)/hardware/src/ $(UNIT_VERILOG)

./build/%.o: $(VERSAT_PC_EMUL)/%.cpp $(HDR) $(UNIT_HDR) $(VERSAT_HDR) $(CPP_FILES) ./build/typeInfo.inc
	mkdir -p ./build
	$(info $@)
	g++ -std=c++11 -DPC -c -o $@ -g -m32 $< -I $(VERSAT_SW_DIR) -I $(VERSAT_PC_EMUL) -I $(VERILATOR_INCLUDE) -I ./build/

./build/structParser.out: $(VERSAT_SW_DIR)/pc-emul/structParser.cpp $(TOOL_COMMON_SRC)
	mkdir -p ./build
	g++ -std=c++11 -DSTANDALONE -o $@ -g -m32 $< -I $(VERSAT_DIR)/software/ -I  $(VERSAT_DIR)/software/pc-emul  -I $(VERSAT_DIR)/software/pc-emul/ $(TOOL_COMMON_SRC)

./build/verilogParser.out: $(VERSAT_SW_DIR)/pc-emul/verilogParser.cpp $(TOOL_SRC) ./build/typeInfo.inc
	mkdir -p ./build
	g++ -std=c++11 -DSTANDALONE -o $@ -g -m32 $< -I ./build/ -I $(VERSAT_DIR)/software/ -I  $(VERSAT_DIR)/software/pc-emul  -I $(VERSAT_DIR)/software/pc-emul/ $(TOOL_SRC)

.PRECIOUS: $(UNIT_HDR)