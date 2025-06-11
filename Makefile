SHELL:=/bin/sh

VERSAT_DIR:=$(shell pwd)

# Default rule
all: versat

include $(VERSAT_DIR)/config.mk

# VERSAT PATHS
VERSAT_SW_DIR:=$(VERSAT_DIR)/software
VERSAT_PC_DIR:=$(VERSAT_SW_DIR)/pc-emul
VERSAT_TOOLS_DIR:=$(VERSAT_SW_DIR)/tools
VERSAT_COMMON_DIR:=$(VERSAT_SW_DIR)/common
VERSAT_TEMPLATE_DIR:=$(VERSAT_SW_DIR)/templates
VERSAT_COMPILER_DIR:=$(VERSAT_SW_DIR)/compiler

BUILD_DIR:=$(VERSAT_DIR)/build
_a := $(shell mkdir -p $(BUILD_DIR)) # Creates the folder

#VERSAT_REQUIRE_TYPE:=type templateEngine

VERSAT_COMMON_HEADERS := $(wildcard $(VERSAT_COMMON_DIR)/*.hpp)
VERSAT_COMMON_SOURCES := $(wildcard $(VERSAT_COMMON_DIR)/*.cpp)
VERSAT_COMMON_OBJS := $(patsubst $(VERSAT_COMMON_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(VERSAT_COMMON_SOURCES))
VERSAT_COMMON_INCLUDE := -I$(VERSAT_COMMON_DIR) -I$(VERSAT_SW_DIR)

VERSAT_ALL_HEADERS := $(VERSAT_COMMON_HEADERS) $(wildcard $(VERSAT_COMPILER_DIR)/*.hpp)
VERSAT_ALL_HEADERS += $(BUILD_DIR)/embeddedData.hpp
VERSAT_ALL_HEADERS += $(BUILD_DIR)/templateData.hpp

VERSAT_TEMPLATES:=$(wildcard $(VERSAT_TEMPLATE_DIR)/*.tpl)

# The fewer files the faster compilation time is
# But the more likely is for type-info to miss something.

TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/templateEngine.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/declaration.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/codeGeneration.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/versatSpecificationParser.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/symbolic.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/addressGen.hpp
TYPE_INFO_HDR += $(BUILD_DIR)/embeddedData.hpp

VERSAT_INCLUDE := -I$(VERSAT_PC_DIR) -I$(VERSAT_COMPILER_DIR) -I$(BUILD_DIR)/ -I$(VERSAT_COMMON_DIR) -I$(VERSAT_SW_DIR)

VERSAT_COMPILER_SOURCES := $(wildcard $(VERSAT_COMPILER_DIR)/*.cpp)
VERSAT_COMPILER_OBJS := $(patsubst $(VERSAT_COMPILER_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(VERSAT_COMPILER_SOURCES))

CPP_OBJ := $(VERSAT_COMMON_OBJS)
CPP_OBJ += $(VERSAT_COMPILER_OBJS)
CPP_OBJ += $(BUILD_DIR)/templateData.o
CPP_OBJ += $(BUILD_DIR)/typeInfo.o
CPP_OBJ += $(BUILD_DIR)/embeddedData.o

COMPILE_TOOL = g++ -DPC -std=c++17 -MMD -MP -DVERSAT_DEBUG -o $@ $< -DROOT_PATH=\"$(abspath $(VERSAT_DIR))\" $(VERSAT_COMMON_FLAGS) $(VERSAT_COMMON_INCLUDE) $(VERSAT_COMMON_OBJS)
COMPILE_OBJ  = g++ -DPC -rdynamic -std=c++17 -MMD -MP -DVERSAT_DEBUG -c -o $@ $< -DROOT_PATH=\"$(abspath $(VERSAT_DIR))\" -g $(VERSAT_COMMON_INCLUDE) $(VERSAT_INCLUDE)

# Common objects (used by Versat and tools)
$(BUILD_DIR)/%.o : $(VERSAT_COMMON_DIR)/%.cpp $(VERSAT_COMMON_HEADERS)
	$(COMPILE_OBJ)

# Compile tools
$(BUILD_DIR)/embedFile: $(VERSAT_TOOLS_DIR)/embedFile.cpp $(VERSAT_COMMON_OBJS) $(VERSAT_COMMON_HEADERS)
	$(COMPILE_TOOL)

$(BUILD_DIR)/calculateHash: $(VERSAT_TOOLS_DIR)/calculateHash.cpp $(VERSAT_COMMON_OBJS) $(VERSAT_COMMON_HEADERS)
	$(COMPILE_TOOL)

$(BUILD_DIR)/embedData: $(VERSAT_TOOLS_DIR)/embedData.cpp $(VERSAT_COMMON_OBJS) $(VERSAT_COMMON_HEADERS)
	$(COMPILE_TOOL)

# Generate meta code
$(BUILD_DIR)/templateData.hpp $(BUILD_DIR)/templateData.cpp: $(VERSAT_TEMPLATES) $(BUILD_DIR)/embedFile
	$(BUILD_DIR)/embedFile $(BUILD_DIR)/templateData $(VERSAT_TEMPLATES)

$(BUILD_DIR)/embeddedData.hpp $(BUILD_DIR)/embeddedData.cpp: $(VERSAT_SW_DIR)/versat_defs.txt $(BUILD_DIR)/embedData
	$(BUILD_DIR)/embedData $(VERSAT_SW_DIR)/versat_defs.txt $(BUILD_DIR)/embeddedData

$(BUILD_DIR)/typeInfo.cpp: $(TYPE_INFO_HDR)
	python3 $(VERSAT_TOOLS_DIR)/structParser.py $(BUILD_DIR) $(TYPE_INFO_HDR) # -m cProfile -o temp.dat 

# Compile object files
$(BUILD_DIR)/templateData.o: $(BUILD_DIR)/templateData.cpp $(BUILD_DIR)/templateData.hpp
	$(COMPILE_OBJ)

$(BUILD_DIR)/embeddedData.o: $(BUILD_DIR)/embeddedData.cpp $(BUILD_DIR)/embeddedData.hpp
	$(COMPILE_OBJ)

$(BUILD_DIR)/typeInfo.o: $(BUILD_DIR)/typeInfo.cpp $(VERSAT_ALL_HEADERS)
	$(COMPILE_OBJ)

$(BUILD_DIR)/%.o : $(VERSAT_COMPILER_DIR)/%.cpp $(VERSAT_ALL_HEADERS)
	$(COMPILE_OBJ)

# Versat
$(VERSAT_DIR)/versat: $(CPP_OBJ) $(VERSAT_ALL_HEADERS)
	g++ -MMD -std=c++17 -DVERSAT_DEBUG -DVERSAT_DIR="$(VERSAT_DIR)" -rdynamic -DROOT_PATH=\"$(abspath $(VERSAT_DIR))\" -o $@ $(VERSAT_COMMON_FLAGS) $(CPP_OBJ) $(VERSAT_INCLUDE) -lstdc++ -lm -lgcc -lc -pthread -ldl 

versat: $(VERSAT_DIR)/versat $(BUILD_DIR)/calculateHash

debug-embed-data: $(BUILD_DIR)/embedData
	gdb --args $(BUILD_DIR)/embedData $(VERSAT_SW_DIR)/versat_defs.txt $(BUILD_DIR)/embeddedData

embed-data: $(BUILD_DIR)/embedData
	$(BUILD_DIR)/embedData $(VERSAT_SW_DIR)/versat_defs.txt $(BUILD_DIR)/embeddedData

clean:
	-rm -fr build
	-rm -f *.a versat versat.d

.PHONY: versat

.SUFFIXES:

