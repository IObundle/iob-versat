#versat common parameters
include $(VERSAT_DIR)/software/software.mk

include $(VERSAT_DIR)/sharedHardware.mk

VERSAT_TEMPLATE_DIR := $(VERSAT_DIR)/software/templates

BUILD_DIR :=./build
#pc sources
HDR+=$(wildcard $(VERSAT_PC_EMUL)/*.hpp)

VERILATE_FLAGS :=-g

INCLUDE += -I$(VERSAT_PC_EMUL)
VERSAT_DEFINE += -DPC

# Add ncurses for debug terminal support
ifeq ($(SUPPORT_NCURSES),1)
  VERSAT_DEFINE +=-DNCURSES
  LIBS +=-lncurses
endif

CPP_FILES := $(wildcard $(VERSAT_PC_EMUL)/*.cpp)
CPP_OBJ := $(patsubst $(VERSAT_PC_EMUL)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))

CPP_FILES += $(wildcard $(VERSAT_DIR)/software/*.cpp)
CPP_OBJ += $(BUILD_DIR)/utilsCommon.o

#Units to verilate
UNIT_VERILOG_BASIC := $(foreach unit,$(VERILATE_UNIT_BASIC),$(VERSAT_DIR)/hardware/src/$(unit).v)

UNIT_HDR+=$(foreach unit,$(VERILATE_UNIT_BASIC),$(BUILD_DIR)/V$(unit).h)
UNIT_HDR+=$(foreach unit,$(VERILATE_UNIT),$(BUILD_DIR)/V$(unit).h)

TYPE_INFO_HDR += $(VERSAT_SW_DIR)/utils.hpp
TYPE_INFO_HDR += $(VERSAT_SW_DIR)/memory.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/versat.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/versatPrivate.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/verilogParser.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/templateEngine.hpp

TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/parser.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/utils.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/memory.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/logger.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/utilsCommon.cpp
TOOL_SRC += $(TOOL_COMMON_SRC)
TOOL_SRC += $(VERSAT_DIR)/software/pc-emul/templateEngine.cpp
TOOL_SRC += $(VERSAT_DIR)/software/pc-emul/type.cpp

CPP_OBJ+=$(BUILD_DIR)/verilated.o
CPP_OBJ+=$(BUILD_DIR)/verilated_vcd_c.o

VERSAT_HDR+=$(wildcard $(VERSAT_SW_DIR)/*.inl)
VERSAT_HDR+=$(wildcard $(VERSAT_PC_EMUL)/*.inl)

$(BUILD_DIR)/verilated.o:
	g++ $(VERILATE_FLAGS) -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -g -c $(VERILATOR_INCLUDE)/verilated.cpp
	mv *.o $(BUILD_DIR)/;
	mv *.d $(BUILD_DIR)/;

$(BUILD_DIR)/verilated_vcd_c.o:
	g++ $(VERILATE_FLAGS) -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -g -c $(VERILATOR_INCLUDE)/verilated_vcd_c.cpp
	mv *.o $(BUILD_DIR)/;
	mv *.d $(BUILD_DIR)/;

$(BUILD_DIR)/V%.h: $(VERSAT_HW_DIR)/src/%.v
	verilator --trace -CFLAGS "$(VERILATE_FLAGS)" -I$(VERSAT_HW_DIR)/src -I$(VERSAT_HW_DIR)/include $(V_INCLUDE) -cc -Mdir $(BUILD_DIR) $<;
	$(MAKE) -C $(BUILD_DIR) -f V$*.mk;

$(BUILD_DIR)/typeInfo.inc: $(BUILD_DIR)/structParser.out $(TYPE_INFO_HDR)
	$(BUILD_DIR)/structParser.out $(BUILD_DIR)/typeInfo.inc $(TYPE_INFO_HDR)

$(BUILD_DIR)/basicWrapper.inc: $(BUILD_DIR)/verilogParser.out $(VERSAT_TEMPLATE_DIR)/unit_verilog_data.tpl
	$(BUILD_DIR)/verilogParser.out Basic $(BUILD_DIR)/basicWrapper.inc -I $(VERSAT_DIR)/submodules/INTERCON/hardware/include/ -I $(VERSAT_DIR)/hardware/include/ -I $(VERSAT_DIR)/hardware/src/ $(UNIT_VERILOG_BASIC)

$(BUILD_DIR)/verilogWrapper.inc: $(BUILD_DIR)/verilogParser.out $(VERSAT_TEMPLATE_DIR)/unit_verilog_data.tpl
	$(BUILD_DIR)/verilogParser.out Verilog $(BUILD_DIR)/verilogWrapper.inc -I $(VERSAT_DIR)/submodules/INTERCON/hardware/include/ -I $(VERSAT_DIR)/hardware/include/ -I $(VERSAT_DIR)/hardware/src/ $(UNIT_VERILOG)

$(BUILD_DIR)/%.o: $(VERSAT_DIR)/software/%.cpp $(HDR) $(UNIT_HDR) $(VERSAT_HDR) $(BUILD_DIR)/typeInfo.inc $(BUILD_DIR)/basicWrapper.inc $(BUILD_DIR)/verilogWrapper.inc
	-g++ -std=c++11 $(VERSAT_DEFINE) -c -o $@ $(GLOBAL_CFLAGS) $< -I $(VERSAT_SW_DIR) -I $(VERSAT_PC_EMUL) -I $(VERILATOR_INCLUDE) -I $(BUILD_DIR)/

$(BUILD_DIR)/%.o: $(VERSAT_PC_EMUL)/%.cpp $(HDR) $(UNIT_HDR) $(VERSAT_HDR) $(BUILD_DIR)/typeInfo.inc $(BUILD_DIR)/basicWrapper.inc $(BUILD_DIR)/verilogWrapper.inc
	-g++ -std=c++11 $(VERSAT_DEFINE) -c -o $@ $(GLOBAL_CFLAGS) $< -I $(VERSAT_SW_DIR) -I $(VERSAT_PC_EMUL) -I $(VERILATOR_INCLUDE) -I $(BUILD_DIR)/

$(BUILD_DIR)/structParser.out: $(VERSAT_SW_DIR)/pc-emul/structParser.cpp $(TOOL_COMMON_SRC)
	g++ -std=c++11 -DPC -DVERSAT_DEBUG -DSTANDALONE -o $@ -g $< -I $(VERSAT_DIR)/software/ -I  $(VERSAT_DIR)/software/pc-emul  -I $(VERSAT_DIR)/software/pc-emul/ $(TOOL_COMMON_SRC)

$(BUILD_DIR)/verilogParser.out: $(VERSAT_SW_DIR)/pc-emul/verilogParser.cpp $(TOOL_SRC) $(BUILD_DIR)/typeInfo.inc
	g++ -std=c++11 -DPC -DVERSAT_DEBUG -DSTANDALONE -o $@ -g $< -I $(BUILD_DIR)/ -I $(VERSAT_DIR)/software/ -I  $(VERSAT_DIR)/software/pc-emul  -I $(VERSAT_DIR)/software/pc-emul/ $(TOOL_SRC)

verilator_test: $(CPP_OBJ)

.PRECIOUS: $(UNIT_HDR)

.NOTPARALLEL: $(BUILD_DIR)/V%.h