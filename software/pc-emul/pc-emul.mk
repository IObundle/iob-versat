#versat common parameters
include $(VERSAT_DIR)/software/software.mk

include $(VERSAT_DIR)/sharedHardware.mk

VERSAT_TEMPLATE_DIR := $(VERSAT_DIR)/software/templates

BUILD_DIR:=./build
VERILATE_DIR:=./verilated
#pc sources
HDR+=$(wildcard $(VERSAT_PC_EMUL)/*.hpp)

VERILATE_FLAGS := -g -fPIC

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

VERSAT_INCLUDE := -I$(VERSAT_SW_DIR) -I$(VERSAT_PC_EMUL) -I$(VERILATOR_INCLUDE) -I$(BUILD_DIR)/ -I$(VERILATE_DIR)/

ifeq ($(DEBUG_GUI),1)
VERSAT_INCLUDE += -I$(VERSAT_PC_EMUL)/IMNODES -I$(VERSAT_PC_EMUL)/IMGUI -I$(VERSAT_PC_EMUL)/IMGUI/backends -I/usr/include/SDL2
endif

VERSAT_DEFINE += -DPC

CPP_FILES := $(wildcard $(VERSAT_PC_EMUL)/*.cpp)
CPP_OBJ := $(patsubst $(VERSAT_PC_EMUL)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))
#CPP_OBJ := $(BUILD_DIR)/unityBuild.o
CPP_OBJ += $(BUILD_DIR)/versat.o

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

#CPP_OBJ+=$(BUILD_DIR)/unityBuild.o

CPP_OBJ+=$(BUILD_DIR)/verilated.o
#CPP_OBJ+=$(BUILD_DIR)/verilated_threads.o
CPP_OBJ+=$(BUILD_DIR)/verilated_vcd_c.o

#Units to verilate
UNIT_VERILOG_BASIC := $(foreach unit,$(VERILATE_UNIT_BASIC),$(VERSAT_DIR)/hardware/src/$(unit).v)

UNIT_HDR+=$(foreach unit,$(VERILATE_UNIT_BASIC),$(VERILATE_DIR)/V$(unit).h)
UNIT_HDR+=$(foreach unit,$(VERILATE_UNIT),$(VERILATE_DIR)/V$(unit).h)

TYPE_INFO_HDR += $(VERSAT_SW_DIR)/utils.hpp
TYPE_INFO_HDR += $(VERSAT_SW_DIR)/utilsCore.hpp
TYPE_INFO_HDR += $(VERSAT_SW_DIR)/memory.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/versat.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/graph.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/declaration.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/versatPrivate.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/configurations.hpp
TYPE_INFO_HDR += $(VERSAT_PC_EMUL)/verilogParsing.hpp
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
TOOL_SRC += $(VERSAT_DIR)/software/pc-emul/unitStats.cpp

TEMPLATES:=$(wildcard $(VERSAT_TEMPLATE_DIR)/*.tpl)

$(BUILD_DIR)/verilated.o: | 
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

$(VERILATE_DIR)/V%.h: $(VERSAT_HW_DIR)/src/%.v
	verilator -DPC=1 $(VERILATOR_FLAGS) --trace -CFLAGS "$(VERILATE_FLAGS)" -I$(VERSAT_HW_DIR)/src -I$(VERSAT_HW_DIR)/include $(V_INCLUDE) -cc -Mdir $(VERILATE_DIR) $<;
	$(MAKE) -C $(VERILATE_DIR) -f V$*.mk;

$(BUILD_DIR)/typeInfo.inc: $(BUILD_DIR)/structParser.out $(TYPE_INFO_HDR)
	$(BUILD_DIR)/structParser.out $(BUILD_DIR)/typeInfo.inc $(TYPE_INFO_HDR)

$(BUILD_DIR)/basicWrapper.inc: $(BUILD_DIR)/verilogParser.out $(BUILD_DIR)/templateData.inc
	$(BUILD_DIR)/verilogParser.out Basic $(BUILD_DIR)/basicWrapper.inc -I $(VERSAT_DIR)/submodules/INTERCON/hardware/include/ -I $(VERSAT_DIR)/hardware/include/ -I $(VERSAT_DIR)/hardware/src/ $(UNIT_VERILOG_BASIC)

$(BUILD_DIR)/verilogWrapper.inc: $(BUILD_DIR)/verilogParser.out $(BUILD_DIR)/templateData.inc
	$(BUILD_DIR)/verilogParser.out Verilog $(BUILD_DIR)/verilogWrapper.inc -I $(VERSAT_DIR)/submodules/INTERCON/hardware/include/ -I $(VERSAT_DIR)/hardware/include/ -I $(VERSAT_DIR)/hardware/src/ $(UNIT_VERILOG)

$(BUILD_DIR)/templateData.inc: $(BUILD_DIR)/embedFile.out $(TEMPLATES)
	$(BUILD_DIR)/embedFile.out $(BUILD_DIR)/templateData.inc $(TEMPLATES) $(TEMPLATES_NAME)

$(BUILD_DIR)/unityBuild.o: $(CPP_FILES) $(VERSAT_SW_DIR)/unityBuild.cpp
	time g++ $(VERSAT_DEFINE) -MMD -c -o $@ $(GLOBAL_CFLAGS) $(VERSAT_SW_DIR)/unityBuild.cpp $(VERSAT_INCLUDE)

$(BUILD_DIR)/versat.o: $(VERSAT_PC_EMUL)/versat.cpp $(HDR) $(UNIT_HDR) $(BUILD_DIR)/basicWrapper.inc $(BUILD_DIR)/verilogWrapper.inc
	time g++ $(VERSAT_DEFINE) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/type.o: $(VERSAT_PC_EMUL)/type.cpp $(HDR) $(UNIT_HDR) $(BUILD_DIR)/typeInfo.inc
	time g++ $(VERSAT_DEFINE) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o: $(VERSAT_SW_DIR)/%.cpp $(HDR)
	time g++ $(VERSAT_DEFINE) -MMD -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o: $(VERSAT_PC_EMUL)/%.cpp $(HDR)
	time g++ $(VERSAT_DEFINE) -MMD -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/structParser.out: $(VERSAT_SW_DIR)/pc-emul/tools/structParser.cpp $(VERSAT_SW_DIR)/pc-emul/structParsing.cpp $(TOOL_COMMON_SRC)
	-g++ -DPC -std=c++17 -DVERSAT_DEBUG -DSTANDALONE -o $@ -g -m32 $(VERSAT_SW_DIR)/pc-emul/tools/structParser.cpp $(VERSAT_SW_DIR)/pc-emul/structParsing.cpp $(VERSAT_INCLUDE) $(TOOL_COMMON_SRC)

$(BUILD_DIR)/embedFile.out: $(VERSAT_SW_DIR)/pc-emul/embedFile.cpp $(TOOL_COMMON_SRC)
	-g++ -DPC -std=c++17 -DVERSAT_DEBUG -DSTANDALONE -o $@ -g -m32 $< $(VERSAT_INCLUDE) $(TOOL_COMMON_SRC)

$(BUILD_DIR)/verilogParser.out: $(VERSAT_SW_DIR)/pc-emul/structParsing.cpp $(VERSAT_SW_DIR)/pc-emul/verilogParsing.cpp $(VERSAT_SW_DIR)/pc-emul/tools/verilogParser.cpp $(TOOL_SRC) $(BUILD_DIR)/templateData.inc $(BUILD_DIR)/typeInfo.inc
	-g++ -DPC -std=c++17 -DVERSAT_DEBUG -DSTANDALONE -o $@ -g -m32 $(VERSAT_SW_DIR)/pc-emul/structParsing.cpp $(VERSAT_SW_DIR)/pc-emul/verilogParsing.cpp $(VERSAT_SW_DIR)/pc-emul/tools/verilogParser.cpp $(VERSAT_INCLUDE) $(TOOL_SRC)

ifeq ($(DEBUG_GUI),1)
$(BUILD_DIR)/%.o: $(VERSAT_PC_EMUL)/IMGUI/%.cpp
	g++ -c -o $@ -D_REENTRANT $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o: $(VERSAT_PC_EMUL)/IMGUI/backends/%.cpp
	g++ -c -o $@ -D_REENTRANT $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o: $(VERSAT_PC_EMUL)/IMNODES/%.cpp
	g++ -c -o $@ -D_REENTRANT $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)
endif

$(shell mkdir -p $(BUILD_DIR))

verilator_test: $(CPP_OBJ)

.PRECIOUS: $(UNIT_HDR)

#.NOTPARALLEL: $(BUILD_DIR)/V%.h

.SUFFIXES:
