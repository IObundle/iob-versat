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

TOOL_BUILD_DIR:=$(VERSAT_DIR)/tool_build
_b := $(shell mkdir -p $(TOOL_BUILD_DIR)) # Creates the folder

#Tools
EMBED := $(TOOL_BUILD_DIR)/embedData
HASH  := $(TOOL_BUILD_DIR)/calculateHash

VERSAT_COMMON_HEADERS := $(wildcard $(VERSAT_COMMON_DIR)/*.hpp)
VERSAT_COMMON_SOURCES := $(wildcard $(VERSAT_COMMON_DIR)/*.cpp)
VERSAT_COMMON_OBJS := $(patsubst $(VERSAT_COMMON_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(VERSAT_COMMON_SOURCES))
VERSAT_COMMON_INCLUDE := -I$(VERSAT_COMMON_DIR) -I$(VERSAT_SW_DIR)

VERSAT_COMMON_TOOLS_OBJS := $(patsubst $(VERSAT_COMMON_DIR)/%.cpp,$(TOOL_BUILD_DIR)/%.o,$(VERSAT_COMMON_SOURCES))

VERSAT_ALL_HEADERS := $(VERSAT_COMMON_HEADERS) $(wildcard $(VERSAT_COMPILER_DIR)/*.hpp)
VERSAT_ALL_HEADERS += $(BUILD_DIR)/embeddedData.hpp

VERSAT_TEMPLATES:=$(wildcard $(VERSAT_TEMPLATE_DIR)/*.tpl)

VERSAT_INCLUDE := -I$(VERSAT_PC_DIR) -I$(VERSAT_COMPILER_DIR) -I$(BUILD_DIR)/ -I$(VERSAT_COMMON_DIR) -I$(VERSAT_SW_DIR)

VERSAT_COMPILER_SOURCES := $(wildcard $(VERSAT_COMPILER_DIR)/*.cpp)
VERSAT_COMPILER_OBJS := $(patsubst $(VERSAT_COMPILER_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(VERSAT_COMPILER_SOURCES))

CPP_OBJ := $(VERSAT_COMMON_OBJS)
CPP_OBJ += $(VERSAT_COMPILER_OBJS)
CPP_OBJ += $(BUILD_DIR)/embeddedData.o

COMPILE_TOOL = g++ -DPC -std=c++17 $(VERSAT_COMMON_FLAGS) -MMD -MP -DVERSAT_DEBUG -o $@ $< -DROOT_PATH=\"$(abspath $(VERSAT_DIR))\" $(VERSAT_COMMON_INCLUDE) $(VERSAT_COMMON_TOOLS_OBJS) -lbfd
COMPILE_TOOL_NO_D = g++ -DPC -std=c++17 $(VERSAT_COMMON_FLAGS) -DVERSAT_DEBUG -o $@ $< -DROOT_PATH=\"$(abspath $(VERSAT_DIR))\" $(VERSAT_COMMON_INCLUDE) $(VERSAT_COMMON_TOOLS_OBJS) -lbfd

COMPILE_OBJ  = g++ -DPC -rdynamic -std=c++17 $(VERSAT_COMMON_FLAGS) -MMD -DVERSAT_DEBUG -c -o $@ $< -DROOT_PATH=\"$(abspath $(VERSAT_DIR))\" -g $(VERSAT_COMMON_INCLUDE) $(VERSAT_INCLUDE)
COMPILE_OBJ_NO_D = g++ -DPC -rdynamic -std=c++17 $(VERSAT_COMMON_FLAGS) -DVERSAT_DEBUG -c -o $@ $< -DROOT_PATH=\"$(abspath $(VERSAT_DIR))\" -g $(VERSAT_COMMON_INCLUDE) $(VERSAT_INCLUDE)

# Common objects (used by Versat and tools)
$(BUILD_DIR)/%.o : $(VERSAT_COMMON_DIR)/%.cpp $(VERSAT_COMMON_HEADERS)
	$(COMPILE_OBJ)

$(TOOL_BUILD_DIR)/%.o : $(VERSAT_COMMON_DIR)/%.cpp $(VERSAT_COMMON_HEADERS)
	$(COMPILE_OBJ)

# Compiler objects
$(BUILD_DIR)/%.o : $(VERSAT_COMPILER_DIR)/%.cpp $(VERSAT_ALL_HEADERS)
	$(COMPILE_OBJ)

# Tools
$(HASH): $(VERSAT_TOOLS_DIR)/calculateHash.cpp $(VERSAT_COMMON_TOOLS_OBJS) $(VERSAT_COMMON_HEADERS)
	$(COMPILE_TOOL)

$(EMBED): $(VERSAT_TOOLS_DIR)/embedData.cpp $(VERSAT_COMMON_TOOLS_OBJS) $(VERSAT_COMMON_HEADERS)
	$(COMPILE_TOOL_NO_D)

# Generate meta code
$(BUILD_DIR)/embeddedData.hpp $(BUILD_DIR)/embeddedData.cpp: $(VERSAT_SW_DIR)/versat_defs.txt $(EMBED)
	$(EMBED) $(VERSAT_SW_DIR)/versat_defs.txt $(BUILD_DIR)/embeddedData

# Compile object files
$(BUILD_DIR)/embeddedData.o: $(BUILD_DIR)/embeddedData.cpp $(BUILD_DIR)/embeddedData.hpp
	$(COMPILE_OBJ_NO_D)

# Embedded data rules will bring the hardware and scripts rules. Any change to hardware and scripts should force make to rebuild through this rule
$(BUILD_DIR)/embeddedData.d: $(VERSAT_SW_DIR)/versat_defs.txt $(EMBED)
	$(EMBED) $(VERSAT_SW_DIR)/versat_defs.txt $(BUILD_DIR)/embeddedData -d

# Versat
$(VERSAT_DIR)/versat: $(CPP_OBJ) $(VERSAT_ALL_HEADERS)
	g++ -MMD -std=c++17 $(VERSAT_COMMON_FLAGS) -DVERSAT_DEBUG -DVERSAT_DIR="$(VERSAT_DIR)" -rdynamic -DROOT_PATH=\"$(abspath $(VERSAT_DIR))\" -o $@ $(CPP_OBJ) $(VERSAT_INCLUDE) -lstdc++ -lm -lgcc -lc -pthread -ldl -lbfd

# TODO: This approach is stupid. There is no point in making the embedData rule file end with a .d format and juggling stuff around so that we do not overwrite our own .d file.
#       Just make it a different ending. Something like .ded and be done with it. Just make sure that the generated .d file from gcc and the .ded file work, because both of them will put different rules for the same file and we might have problems. 
-include $(BUILD_DIR)/embeddedData.d
-include $(BUILD_DIR)/*.d

versat: $(VERSAT_DIR)/versat $(HASH)

debug-embed-data: $(EMBED)
	gdb --args $(EMBED) $(VERSAT_SW_DIR)/versat_defs.txt $(TOOL_BUILD_DIR)/embeddedData

embed-data: $(EMBED)
	$(EMBED) $(VERSAT_SW_DIR)/versat_defs.txt $(TOOL_BUILD_DIR)/embeddedData

clean:
	-rm -fr $(BUILD_DIR)
	-rm -f *.a versat versat.d

clean-all: clean
	-rm -fr $(TOOL_BUILD_DIR)

.PHONY: versat $(BUILD_DIR)/embeddedData.d

.SUFFIXES:

