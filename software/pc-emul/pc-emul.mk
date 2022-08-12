#versat common parameters
include $(VERSAT_DIR)/software/software.mk

VERSAT_TEMPLATE_DIR := $(VERSAT_DIR)/software/templates

BUILD_DIR=./build
#pc sources
HDR+=$(wildcard $(VERSAT_PC_EMUL)/*.hpp)

VERILATE_FLAGS=-g -m32

INCLUDE+= -I$(VERSAT_PC_EMUL)
DEFINE+= -DPC

CPP_FILES = $(wildcard $(VERSAT_PC_EMUL)/*.cpp)
CPP_OBJ = $(patsubst $(VERSAT_PC_EMUL)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))

#Units to verilate
VERILATE_UNIT := Reg Mem Muladd VRead VWrite PipelineRegister Mux2 Merge Const Delay
UNIT_VERILOG := $(foreach unit,$(VERILATE_UNIT),$(VERSAT_DIR)/hardware/src/$(unit).v)

UNIT_HDR+=$(foreach unit,$(VERILATE_UNIT),$(BUILD_DIR)/V$(unit).h)

TYPE_INFO_HDR = $(VERSAT_PC_EMUL)/versatPrivate.hpp $(VERSAT_SW_DIR)/utils.hpp $(VERSAT_PC_EMUL)/verilogParser.hpp $(VERSAT_PC_EMUL)/templateEngine.hpp $(VERSAT_PC_EMUL)/memory.hpp

TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/parser.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/utils.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/memory.cpp
TOOL_SRC += $(TOOL_COMMON_SRC)
TOOL_SRC += $(VERSAT_DIR)/software/pc-emul/templateEngine.cpp
TOOL_SRC += $(VERSAT_DIR)/software/pc-emul/type.cpp

CPP_OBJ+=$(BUILD_DIR)/verilated.o
CPP_OBJ+=$(BUILD_DIR)/verilated_vcd_c.o

$(BUILD_DIR)/verilated.o:
	mkdir -p $(BUILD_DIR);
	g++ $(VERILATE_FLAGS) -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -m32 -g -c $(VERILATOR_INCLUDE)/verilated.cpp
	mv *.o $(BUILD_DIR)/;
	mv *.d $(BUILD_DIR)/;

$(BUILD_DIR)/verilated_vcd_c.o:
	mkdir -p $(BUILD_DIR);
	g++ $(VERILATE_FLAGS) -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -m32 -g -c $(VERILATOR_INCLUDE)/verilated_vcd_c.cpp
	mv *.o $(BUILD_DIR)/;
	mv *.d $(BUILD_DIR)/;

$(BUILD_DIR)/V%.h: $(VERSAT_HW_DIR)/src/%.v
	verilator --trace -CFLAGS "$(VERILATE_FLAGS)" -I$(VERSAT_HW_DIR)/src -I$(VERSAT_HW_DIR)/include -I$(VERSAT_DIR)/submodules/MEM/ram/2p_ram -I$(VERSAT_DIR)/submodules/MEM/ram/tdp_ram -cc -Mdir $(BUILD_DIR) $<;
	cd $(BUILD_DIR) && make -f V$*.mk;

$(BUILD_DIR)/typeInfo.inc: $(BUILD_DIR)/structParser.out $(TYPE_INFO_HDR)
	mkdir -p $(BUILD_DIR)
	$(BUILD_DIR)/structParser.out $(BUILD_DIR)/typeInfo.inc $(TYPE_INFO_HDR)

$(BUILD_DIR)/verilogWrapper.inc: $(BUILD_DIR)/verilogParser.out  $(VERSAT_SW_DIR)/pc-emul/verilogParser.cpp $(VERSAT_TEMPLATE_DIR)/unit_verilog_data.tpl
	$(BUILD_DIR)/verilogParser.out $(BUILD_DIR)/verilogWrapper.inc -I $(VERSAT_DIR)/submodules/INTERCON/hardware/include/ -I $(VERSAT_DIR)/hardware/include/ -I $(VERSAT_DIR)/hardware/src/ $(UNIT_VERILOG)

$(BUILD_DIR)/%.o: $(VERSAT_PC_EMUL)/%.cpp $(HDR) $(UNIT_HDR) $(VERSAT_HDR) $(BUILD_DIR)/typeInfo.inc $(BUILD_DIR)/verilogWrapper.inc
	mkdir -p $(BUILD_DIR)
	g++ -std=c++11 -DPC -c -o $@ $(GLOBAL_CFLAGS) $< -I $(VERSAT_SW_DIR) -I $(VERSAT_PC_EMUL) -I $(VERILATOR_INCLUDE) -I $(BUILD_DIR)/

$(BUILD_DIR)/structParser.out: $(VERSAT_SW_DIR)/pc-emul/structParser.cpp $(TOOL_COMMON_SRC)
	mkdir -p $(BUILD_DIR)
	g++ -std=c++11 -DSTANDALONE -o $@ -g -m32 $< -I $(VERSAT_DIR)/software/ -I  $(VERSAT_DIR)/software/pc-emul  -I $(VERSAT_DIR)/software/pc-emul/ $(TOOL_COMMON_SRC)

$(BUILD_DIR)/verilogParser.out: $(VERSAT_SW_DIR)/pc-emul/verilogParser.cpp $(TOOL_SRC) $(BUILD_DIR)/typeInfo.inc
	mkdir -p $(BUILD_DIR)
	g++ -std=c++11 -DSTANDALONE -o $@ -g -m32 $< -I $(BUILD_DIR)/ -I $(VERSAT_DIR)/software/ -I  $(VERSAT_DIR)/software/pc-emul  -I $(VERSAT_DIR)/software/pc-emul/ $(TOOL_SRC)

.PRECIOUS: $(UNIT_HDR)
