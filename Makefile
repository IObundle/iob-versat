SHELL:=/bin/sh

VERSAT_DIR:=$(shell pwd)

# Default rule
all: versat

include $(VERSAT_DIR)/config.mk

# VERSAT PATHS
VERSAT_HW_DIR:=$(VERSAT_DIR)/hardware
VERSAT_SW_DIR:=$(VERSAT_DIR)/software
VERSAT_COMP_DIR:=$(VERSAT_SW_DIR)/compiler
VERSAT_SUBMODULES_DIR:=$(VERSAT_DIR)/submodules
VERSAT_PC_DIR:=$(VERSAT_SW_DIR)/pc-emul
VERSAT_TOOLS_DIR:=$(VERSAT_SW_DIR)/tools
VERSAT_COMMON_DIR:=$(VERSAT_SW_DIR)/common
VERSAT_TEMPLATE_DIR:=$(VERSAT_SW_DIR)/templates
VERSAT_COMPILER_DIR:=$(VERSAT_SW_DIR)/compiler

BUILD_DIR:=$(VERSAT_DIR)/build
_a := $(shell mkdir -p $(BUILD_DIR)) # Creates the folder

VERSAT_REQUIRE_TYPE:=type verilogParsing templateEngine

VERSAT_COMMON_SRC_NO_TYPE := $(filter-out $(patsubst %,$(VERSAT_COMMON_DIR)/%.cpp,$(VERSAT_REQUIRE_TYPE)),$(wildcard $(VERSAT_COMMON_DIR)/*.cpp))
VERSAT_COMMON_HDR_NO_TYPE := $(filter-out $(patsubst %,$(VERSAT_COMMON_DIR)/%.hpp,$(VERSAT_REQUIRE_TYPE)),$(wildcard $(VERSAT_COMMON_DIR)/*.hpp))
VERSAT_COMMON_OBJ_NO_TYPE := $(patsubst $(VERSAT_COMMON_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(VERSAT_COMMON_SRC_NO_TYPE))

VERSAT_COMMON_SRC:=$(VERSAT_COMMON_SRC_NO_TYPE) $(patsubst %,$(VERSAT_COMMON_DIR)/%.cpp,$(VERSAT_REQUIRE_TYPE))
VERSAT_COMMON_SRC+=$(BUILD_DIR)/repr.cpp
VERSAT_COMMON_SRC+=$(BUILD_DIR)/repr.hpp
VERSAT_COMMON_SRC+=$(BUILD_DIR)/repr.o

#VERSAT_COMMON_HDR:=$(VERSAT_COMMON_HDR_NO_TYPE) $(patsubst %,$(VERSAT_COMMON_DIR)/%.hpp,$(VERSAT_REQUIRE_TYPE))
VERSAT_COMMON_OBJ:=$(VERSAT_COMMON_OBJ_NO_TYPE) $(patsubst %,$(BUILD_DIR)/%.o,$(VERSAT_REQUIRE_TYPE))

VERSAT_COMMON_INCLUDE := -I$(VERSAT_COMMON_DIR) -I$(VERSAT_SW_DIR)

# After first rule, only build needed objects
VERSAT_COMMON_DEPENDS := $(patsubst %.o,%.d,$(VERSAT_COMMON_OBJ))
-include  $(VERSAT_COMMON_DEPENDS)

VERSAT_DEFINE += -DPC

# -rdynamic - used to allow backtraces
$(BUILD_DIR)/%.o : $(VERSAT_COMMON_DIR)/%.cpp
	-g++ $(VERSAT_DEFINE) -MMD -rdynamic $(VERSAT_COMMON_FLAGS) -std=c++17 -c -o $@ -DROOT_PATH=\"$(abspath ../)\" -DVERSAT_DEBUG $(GLOBAL_CFLAGS) $< $(VERSAT_COMMON_INCLUDE)

VERSAT_TEMPLATES:=$(wildcard $(VERSAT_TEMPLATE_DIR)/*.tpl)
VERSAT_TEMPLATES_OBJ:=$(BUILD_DIR)/templateData.o

TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/utils.hpp
TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/utilsCore.hpp
TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/memory.hpp
TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/templateEngine.hpp
TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/parser.hpp
TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/verilogParsing.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/accelerator.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/versat.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/declaration.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/configurations.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/codeGeneration.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/versatSpecificationParser.hpp

VERSAT_INCLUDE += -I$(VERSAT_PC_DIR) -I$(VERSAT_COMPILER_DIR) -I$(BUILD_DIR)/ -I$(VERSAT_COMMON_DIR)

VERSAT_DEBUG:=1

BUILD_DIR:=$(VERSAT_DIR)/build

VERSAT_LIBS:=-lstdc++ -lm -lgcc -lc -pthread -ldl

#pc headers
VERSAT_HDR+=$(wildcard $(VERSAT_PC_DIR)/*.hpp)
VERSAT_HDR+=$(wildcard $(VERSAT_COMPILER_DIR)/*.hpp)

VERSAT_INCLUDE += -I$(VERSAT_PC_DIR) -I$(VERSAT_COMPILER_DIR) -I$(BUILD_DIR)/ -I$(VERSAT_COMMON_DIR) -I$(VERSAT_SW_DIR)

CPP_FILES := $(wildcard $(VERSAT_COMPILER_DIR)/*.cpp)
VERSAT_COMPILER_OBJ := $(patsubst $(VERSAT_COMPILER_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))
CPP_OBJ := $(VERSAT_COMPILER_OBJ)

# -rdynamic needed to ensure backtraces work
VERSAT_FLAGS := -rdynamic -DROOT_PATH=\"$(abspath ../)\" 

CPP_OBJ += $(VERSAT_COMMON_OBJ)
CPP_OBJ += $(VERSAT_TEMPLATES_OBJ)
CPP_OBJ += $(BUILD_DIR)/typeInfo.o
CPP_OBJ += $(BUILD_DIR)/autoRepr.o

CPP_OBJ_WITHOUT_COMPILER:=$(filter-out $(BUILD_DIR)/versatCompiler.o,$(CPP_OBJ))

FL:=-DVERSAT_DIR="$(VERSAT_DIR)"

VERSAT_COMPILER_DEPENDS := $(patsubst %.o,%.d,$(VERSAT_COMPILER_OBJ))
-include  $(VERSAT_COMPILER_DEPENDS)

$(BUILD_DIR)/templateData.hpp $(BUILD_DIR)/templateData.cpp &: $(VERSAT_TEMPLATES) $(BUILD_DIR)/embedFile
	$(BUILD_DIR)/embedFile $(BUILD_DIR)/templateData $(VERSAT_TEMPLATES)

$(BUILD_DIR)/templateData.o: $(BUILD_DIR)/templateData.cpp $(BUILD_DIR)/templateData.hpp
	-g++ -DPC -std=c++17 -MMD -MP -DVERSAT_DEBUG -DSTANDALONE -c -o $@ -g $(VERSAT_COMMON_INCLUDE) $<

$(BUILD_DIR)/embedFile: $(VERSAT_TOOLS_DIR)/embedFile.cpp $(VERSAT_COMMON_OBJ_NO_TYPE)
	-g++ -DPC -std=c++17 -MMD -MP -DVERSAT_DEBUG -DSTANDALONE -o $@ $(VERSAT_COMMON_FLAGS) $(VERSAT_COMMON_INCLUDE) $< $(VERSAT_COMMON_OBJ_NO_TYPE)

$(VERSAT_DIR)/software/autoRepr.cpp $(VERSAT_DIR)/software/typeInfo.cpp: $(TYPE_INFO_HDR)
	python3 $(VERSAT_TOOLS_DIR)/structParser.py $(VERSAT_DIR)/software $(TYPE_INFO_HDR) || true

$(BUILD_DIR)/typeInfo.o: $(VERSAT_DIR)/software/typeInfo.cpp
	-g++ -DPC -MMD -MP -std=c++17 $(VERSAT_COMMON_FLAGS) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/autoRepr.o: $(VERSAT_DIR)/software/autoRepr.cpp
	-g++ -DPC -MMD -MP -std=c++17 $(VERSAT_COMMON_FLAGS) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o : $(VERSAT_COMPILER_DIR)/%.cpp $(BUILD_DIR)/templateData.hpp
	-g++ -MMD -MP -std=c++17 $(VERSAT_FLAGS) $(FL) $(VERSAT_COMMON_FLAGS) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE) #-DDEFAULT_UNIT_PATHS="$(SHARED_UNITS_PATH)" 

$(shell mkdir -p $(BUILD_DIR))

$(VERSAT_DIR)/versat: $(CPP_OBJ) 
	-g++ -MMD -MP -std=c++17 $(FL) $(VERSAT_FLAGS) -o $@ $(VERSAT_COMMON_FLAGS) $(CPP_OBJ) $(VERSAT_INCLUDE) $(VERSAT_LIBS) 

embedFile: $(BUILD_DIR)/templateData.hpp

versat: $(VERSAT_DIR)/versat
	#$(MAKE) -C $(VERSAT_COMP_DIR) versat

type-info: $(TYPE_INFO_HDR)
	python3 $(VERSAT_TOOLS_DIR)/structParser.py $(VERSAT_DIR)/software $(TYPE_INFO_HDR) || true

clean:
	-rm -fr build
	-rm -f *.a versat versat.d

.PHONY: versat

.SUFFIXES:

