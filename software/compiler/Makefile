SHELL:=/bin/sh

VERSAT_DIR:=$(abspath $(shell pwd)/../..)

#versat common parameters
include $(VERSAT_DIR)/config.mk
include $(VERSAT_TOOLS_DIR)/tools.mk
include $(VERSAT_DIR)/sharedHardware.mk

VERSAT_DEBUG:=1

BUILD_DIR:=$(VERSAT_DIR)/build

VERSAT_LIBS:=-lstdc++ -lm -lgcc -lc -pthread -ldl

#pc headers
VERSAT_HDR+=$(wildcard $(VERSAT_PC_DIR)/*.hpp)
VERSAT_HDR+=$(wildcard $(VERSAT_COMPILER_DIR)/*.hpp)

VERILATE_FLAGS := -g -fPIC

VERSAT_INCLUDE += -I$(VERSAT_PC_DIR) -I$(VERSAT_COMPILER_DIR) -I$(BUILD_DIR)/ -I$(VERSAT_COMMON_DIR)
VERSAT_DEFINE += -DPC

CPP_FILES := $(wildcard $(VERSAT_COMPILER_DIR)/*.cpp)
VERSAT_COMPILER_OBJ := $(patsubst $(VERSAT_COMPILER_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))
CPP_OBJ := $(VERSAT_COMPILER_OBJ)

VERSAT_FLAGS := -rdynamic -DROOT_PATH=\"$(abspath ../)\" 

CPP_OBJ += $(VERSAT_COMMON_OBJ)
CPP_OBJ += $(VERSAT_TEMPLATES_OBJ)
CPP_OBJ += $(BUILD_DIR)/typeInfo.o
CPP_OBJ += $(BUILD_DIR)/repr.o

CPP_OBJ_WITHOUT_COMPILER:=$(filter-out $(BUILD_DIR)/versatCompiler.o,$(CPP_OBJ))

FL:=-DVERSAT_DIR="$(VERSAT_DIR)"

$(VERSAT_DIR)/versat: $(CPP_OBJ)
	-g++ -MMD -MP -std=c++17 $(FL) $(VERSAT_FLAGS) -Wswitch-enum -o $@ -g $(CPP_OBJ) $(VERSAT_INCLUDE) $(VERSAT_LIBS) 

versat: $(VERSAT_DIR)/versat

VERSAT_COMPILER_DEPENDS := $(patsubst %.o,%.d,$(VERSAT_COMPILER_OBJ))
-include  $(VERSAT_COMPILER_DEPENDS)

$(BUILD_DIR)/%.o : $(VERSAT_COMPILER_DIR)/%.cpp
	-g++ -MMD -MP -std=c++17 -Wswitch-enum $(VERSAT_FLAGS) $(FL) -g -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE) #-DDEFAULT_UNIT_PATHS="$(SHARED_UNITS_PATH)" 

$(shell mkdir -p $(BUILD_DIR))

verilator_test: $(CPP_OBJ)

.PHONY: versat

.SUFFIXES:


