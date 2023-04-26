#versat common parameters
include $(VERSAT_DIR)/software/software.mk

include $(VERSAT_DIR)/sharedHardware.mk

VERSAT_TEMPLATE_DIR := $(VERSAT_DIR)/software/templates

BUILD_DIR :=./build
#pc sources
HDR+=$(wildcard $(VERSAT_PC_EMUL)/*.hpp)

VERILATE_FLAGS := -g

INCLUDE += -I$(VERSAT_PC_EMUL)

ifeq ($(DEBUG_GUI),1)
INCLUDE += -I$(VERSAT_PC_EMUL)/IMGUI
INCLUDE += -I$(VERSAT_PC_EMUL)/IMNODES

VERSAT_DEFINE += -DDEBUG_GUI
VERILATOR_FLAGS += -Dx64
else
VERSAT_DEFINE += -Dx86 
VERILATE_FLAGS += -m32
VERILATOR_FLAGS += -Dx86
endif

VERSAT_INCLUDE := -I$(VERSAT_SW_DIR) -I$(VERSAT_PC_EMUL) -I$(VERILATOR_INCLUDE) -I$(BUILD_DIR)/ 

ifeq ($(DEBUG_GUI),1)
VERSAT_INCLUDE += -I$(VERSAT_PC_EMUL)/IMNODES -I$(VERSAT_PC_EMUL)/IMGUI -I$(VERSAT_PC_EMUL)/IMGUI/backends -I/usr/include/SDL2
endif

VERSAT_DEFINE += -DPC

CPP_FILES := $(wildcard $(VERSAT_PC_EMUL)/*.cpp)
CPP_OBJ := $(patsubst $(VERSAT_PC_EMUL)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))

ifeq ($(DEBUG_GUI),1)
IMGUI_FILES := $(wildcard $(VERSAT_PC_EMUL)/IMGUI/*.cpp)
IMGUI_OBJ := $(patsubst $(VERSAT_PC_EMUL)/IMGUI/%.cpp,$(BUILD_DIR)/%.o,$(IMGUI_FILES))

IMGUI_BACKEND_FILES := $(VERSAT_PC_EMUL)/IMGUI/backends/imgui_impl_opengl3.cpp $(VERSAT_PC_EMUL)/IMGUI/backends/imgui_impl_sdl.cpp
IMGUI_BACKEND_OBJ := $(patsubst $(VERSAT_PC_EMUL)/IMGUI/backends/%.cpp,$(BUILD_DIR)/%.o,$(IMGUI_BACKEND_FILES))

IMNODES_FILES := $(wildcard $(VERSAT_PC_EMUL)/IMNODES/*.cpp)
IMNODES_OBJ := $(patsubst $(VERSAT_PC_EMUL)/IMNODES/%.cpp,$(BUILD_DIR)/%.o,$(IMNODES_FILES))

CPP_FILES += $(IMGUI_FILES) $(IMGUI_BACKEND_FILES) $(IMNODES_FILES)
CPP_OBJ += $(IMGUI_OBJ) $(IMGUI_BACKEND_OBJ) $(IMNODES_OBJ)
endif

CPP_FILES += $(VERSAT_DIR)/software/utilsCommon.cpp
CPP_OBJ += $(BUILD_DIR)/utilsCommon.o

#Units to verilate
UNIT_VERILOG_BASIC := $(foreach unit,$(VERILATE_UNIT_BASIC),$(VERSAT_DIR)/hardware/src/$(unit).v)

UNIT_HDR+=$(foreach unit,$(VERILATE_UNIT_BASIC),$(BUILD_DIR)/V$(unit).h)
UNIT_HDR+=$(foreach unit,$(VERILATE_UNIT),$(BUILD_DIR)/V$(unit).h)

TYPE_INFO_HDR += $(VERSAT_SW_DIR)/utils.hpp
TYPE_INFO_HDR += $(VERSAT_SW_DIR)/utilsCore.hpp
TYPE_INFO_HDR += $(VERSAT_SW_DIR)/memory.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/versat.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/versatPrivate.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/verilogParser.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/templateEngine.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/parser.hpp

TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/parser.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/utils.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/memory.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/pc-emul/logger.cpp
TOOL_COMMON_SRC += $(VERSAT_DIR)/software/utilsCommon.cpp
TOOL_SRC += $(TOOL_COMMON_SRC)
TOOL_SRC += $(VERSAT_DIR)/software/pc-emul/templateEngine.cpp
TOOL_SRC += $(VERSAT_DIR)/software/pc-emul/type.cpp

CPP_OBJ+=$(BUILD_DIR)/verilated.o
#CPP_OBJ+=$(BUILD_DIR)/verilated_threads.o
CPP_OBJ+=$(BUILD_DIR)/verilated_vcd_c.o

VERSAT_HDR+=$(wildcard $(VERSAT_SW_DIR)/*.inl)
VERSAT_HDR+=$(wildcard $(VERSAT_PC_EMUL)/*.inl)

$(BUILD_DIR)/verilated.o:
	g++ $(VERILATE_FLAGS) -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -g -c $(VERILATOR_INCLUDE)/verilated.cpp
	mv *.o $(BUILD_DIR)/;
	mv *.d $(BUILD_DIR)/;

$(BUILD_DIR)/verilated_threads.o:
	mkdir -p $(BUILD_DIR);
	g++ $(VERILATE_FLAGS) -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -m32 -g -c $(VERILATOR_INCLUDE)/verilated_threads.cpp
	mv *.o $(BUILD_DIR)/;
	mv *.d $(BUILD_DIR)/;

$(BUILD_DIR)/verilated_vcd_c.o:
	g++ $(VERILATE_FLAGS) -I. -MMD -I$(VERILATOR_INCLUDE) -I$(VERILATOR_INCLUDE)/vltstd -DVL_PRINTF=printf \
	-DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable \
	-Wno-unused-parameter -Wno-unused-variable -Wno-shadow -g -c $(VERILATOR_INCLUDE)/verilated_vcd_c.cpp
	mv *.o $(BUILD_DIR)/;
	mv *.d $(BUILD_DIR)/;

$(BUILD_DIR)/V%.h: $(VERSAT_HW_DIR)/src/%.v
	verilator -DPC=1 $(VERILATOR_FLAGS) --trace -CFLAGS "$(VERILATE_FLAGS)" -I$(VERSAT_HW_DIR)/src -I$(VERSAT_HW_DIR)/include $(V_INCLUDE) -cc -Mdir $(BUILD_DIR) $<;
	$(MAKE) -C $(BUILD_DIR) -f V$*.mk;

$(BUILD_DIR)/typeInfo.inc: $(BUILD_DIR)/structParser.out $(TYPE_INFO_HDR)
	$(BUILD_DIR)/structParser.out $(BUILD_DIR)/typeInfo.inc $(TYPE_INFO_HDR)

$(BUILD_DIR)/basicWrapper.inc: $(BUILD_DIR)/verilogParser.out $(VERSAT_TEMPLATE_DIR)/unit_verilog_data.tpl
	$(BUILD_DIR)/verilogParser.out Basic $(BUILD_DIR)/basicWrapper.inc -I $(VERSAT_DIR)/submodules/INTERCON/hardware/include/ -I $(VERSAT_DIR)/hardware/include/ -I $(VERSAT_DIR)/hardware/src/ $(UNIT_VERILOG_BASIC)

$(BUILD_DIR)/verilogWrapper.inc: $(BUILD_DIR)/verilogParser.out $(VERSAT_TEMPLATE_DIR)/unit_verilog_data.tpl
	$(BUILD_DIR)/verilogParser.out Verilog $(BUILD_DIR)/verilogWrapper.inc -I $(VERSAT_DIR)/submodules/INTERCON/hardware/include/ -I $(VERSAT_DIR)/hardware/include/ -I $(VERSAT_DIR)/hardware/src/ $(UNIT_VERILOG)

$(BUILD_DIR)/%.o: $(VERSAT_SW_DIR)/%.cpp $(HDR) $(UNIT_HDR) $(VERSAT_HDR) $(BUILD_DIR)/typeInfo.inc $(BUILD_DIR)/basicWrapper.inc $(BUILD_DIR)/verilogWrapper.inc
	-g++ $(VERSAT_DEFINE) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o: $(VERSAT_PC_EMUL)/%.cpp $(HDR) $(UNIT_HDR) $(VERSAT_HDR) $(BUILD_DIR)/typeInfo.inc $(BUILD_DIR)/basicWrapper.inc $(BUILD_DIR)/verilogWrapper.inc
	-g++ $(VERSAT_DEFINE) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/structParser.out: $(VERSAT_SW_DIR)/pc-emul/structParser.cpp $(TOOL_COMMON_SRC)
	-g++ -DPC -DVERSAT_DEBUG -DSTANDALONE -o $@ -g -m32 $< $(VERSAT_INCLUDE) $(TOOL_COMMON_SRC)

$(BUILD_DIR)/verilogParser.out: $(VERSAT_SW_DIR)/pc-emul/verilogParser.cpp $(TOOL_SRC) $(BUILD_DIR)/typeInfo.inc
	-g++ -DPC -DVERSAT_DEBUG -DSTANDALONE -o $@ -g -m32 $< $(VERSAT_INCLUDE) $(TOOL_SRC)

ifeq ($(DEBUG_GUI),1)
$(BUILD_DIR)/%.o: $(VERSAT_PC_EMUL)/IMGUI/%.cpp
	g++ -c -o $@ -D_REENTRANT $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o: $(VERSAT_PC_EMUL)/IMGUI/backends/%.cpp
	g++ -c -o $@ -D_REENTRANT $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o: $(VERSAT_PC_EMUL)/IMNODES/%.cpp
	g++ -c -o $@ -D_REENTRANT $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)
endif

verilator_test: $(CPP_OBJ)

.PRECIOUS: $(UNIT_HDR)

.NOTPARALLEL: $(BUILD_DIR)/V%.h

.SUFFIXES:
