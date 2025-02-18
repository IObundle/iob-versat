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

VERSAT_REQUIRE_TYPE:=type templateEngine

VERSAT_ALL_HEADERS := $(wildcard $(VERSAT_COMMON_DIR)/*.hpp) $(wildcard $(VERSAT_COMPILER_DIR)/*.hpp)
VERSAT_COMMON_SRC_NO_TYPE := $(filter-out $(patsubst %,$(VERSAT_COMMON_DIR)/%.cpp,$(VERSAT_REQUIRE_TYPE)),$(wildcard $(VERSAT_COMMON_DIR)/*.cpp))
VERSAT_COMMON_HDR_NO_TYPE := $(filter-out $(patsubst %,$(VERSAT_COMMON_DIR)/%.hpp,$(VERSAT_REQUIRE_TYPE)),$(wildcard $(VERSAT_COMMON_DIR)/*.hpp))
VERSAT_COMMON_OBJ_NO_TYPE := $(patsubst $(VERSAT_COMMON_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(VERSAT_COMMON_SRC_NO_TYPE))

VERSAT_COMMON_OBJ:=$(VERSAT_COMMON_OBJ_NO_TYPE) $(patsubst %,$(BUILD_DIR)/%.o,$(VERSAT_REQUIRE_TYPE))

VERSAT_COMMON_INCLUDE := -I$(VERSAT_COMMON_DIR) -I$(VERSAT_SW_DIR)

VERSAT_DEFINE += -DPC

VERSAT_TEMPLATES:=$(wildcard $(VERSAT_TEMPLATE_DIR)/*.tpl)
VERSAT_TEMPLATES_OBJ:=$(BUILD_DIR)/templateData.o

# The fewer files the faster compilation time is
# But the more likely is for type-info to miss something.

TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/templateEngine.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/declaration.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/codeGeneration.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/versatSpecificationParser.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/symbolic.hpp

VERSAT_INCLUDE := -I$(VERSAT_PC_DIR) -I$(VERSAT_COMPILER_DIR) -I$(BUILD_DIR)/ -I$(VERSAT_COMMON_DIR) -I$(VERSAT_SW_DIR)

VERSAT_DEBUG:=1

BUILD_DIR:=$(VERSAT_DIR)/build

VERSAT_LIBS:=-lstdc++ -lm -lgcc -lc -pthread -ldl

CPP_FILES := $(wildcard $(VERSAT_COMPILER_DIR)/*.cpp)
VERSAT_COMPILER_OBJ := $(patsubst $(VERSAT_COMPILER_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))
CPP_OBJ := $(VERSAT_COMPILER_OBJ)

# -rdynamic needed to ensure backtraces work
VERSAT_FLAGS := -rdynamic -DROOT_PATH=\"$(abspath ../)\" 

CPP_OBJ += $(VERSAT_COMMON_OBJ)
CPP_OBJ += $(VERSAT_TEMPLATES_OBJ)
CPP_OBJ += $(BUILD_DIR)/typeInfo.o

CPP_OBJ_WITHOUT_COMPILER:=$(filter-out $(BUILD_DIR)/versatCompiler.o,$(CPP_OBJ))

FL:=-DVERSAT_DIR="$(VERSAT_DIR)"

VERSAT_COMPILER_DEPENDS := $(patsubst %.o,%.d,$(VERSAT_COMPILER_OBJ))
-include  $(VERSAT_COMPILER_DEPENDS)

$(BUILD_DIR)/templateData.hpp $(BUILD_DIR)/templateData.cpp &: $(VERSAT_TEMPLATES) $(BUILD_DIR)/embedFile $(VERSAT_ALL_HEADERS)
	$(BUILD_DIR)/embedFile $(BUILD_DIR)/templateData $(VERSAT_TEMPLATES)

$(BUILD_DIR)/templateData.o: $(BUILD_DIR)/templateData.cpp $(BUILD_DIR)/templateData.hpp $(VERSAT_ALL_HEADERS)
	g++ -DPC -std=c++17 -MMD -MP -DVERSAT_DEBUG -DSTANDALONE -c -o $@ -g $(VERSAT_COMMON_INCLUDE) $<

$(BUILD_DIR)/embedFile: $(VERSAT_TOOLS_DIR)/embedFile.cpp $(VERSAT_COMMON_OBJ_NO_TYPE) $(VERSAT_ALL_HEADERS)
	g++ -DPC -std=c++17 -MMD -MP -DVERSAT_DEBUG -DSTANDALONE -o $@ $(VERSAT_COMMON_FLAGS) $(VERSAT_COMMON_INCLUDE) $< $(VERSAT_COMMON_OBJ_NO_TYPE)

$(BUILD_DIR)/calculateHash: $(VERSAT_TOOLS_DIR)/calculateHash.cpp $(VERSAT_COMMON_OBJ_NO_TYPE) $(VERSAT_ALL_HEADERS)
	g++ -DPC -std=c++17 -MMD -MP -DVERSAT_DEBUG -DSTANDALONE -o $@ $(VERSAT_COMMON_FLAGS) $(VERSAT_COMMON_INCLUDE) $< $(VERSAT_COMMON_OBJ_NO_TYPE)

$(BUILD_DIR)/typeInfo.cpp: $(TYPE_INFO_HDR) $(VERSAT_ALL_HEADERS)
	python3 $(VERSAT_TOOLS_DIR)/structParser.py $(BUILD_DIR) $(TYPE_INFO_HDR) # -m cProfile -o temp.dat 

$(BUILD_DIR)/typeInfo.o: $(BUILD_DIR)/typeInfo.cpp $(VERSAT_ALL_HEADERS)
	g++ -DPC -std=c++17 $(VERSAT_COMMON_FLAGS) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o : $(VERSAT_COMPILER_DIR)/%.cpp $(BUILD_DIR)/templateData.hpp $(VERSAT_ALL_HEADERS)
	g++ -MMD -std=c++17 $(VERSAT_FLAGS) $(FL) $(VERSAT_COMMON_FLAGS) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

# -rdynamic - used to allow backtraces
$(BUILD_DIR)/%.o : $(VERSAT_COMMON_DIR)/%.cpp $(VERSAT_ALL_HEADERS)
	g++ $(VERSAT_DEFINE) -rdynamic $(VERSAT_COMMON_FLAGS) -std=c++17 -c -o $@ -DROOT_PATH=\"$(abspath ../)\" -DVERSAT_DEBUG $(GLOBAL_CFLAGS) $< $(VERSAT_COMMON_INCLUDE)

$(VERSAT_DIR)/versat: $(CPP_OBJ) $(VERSAT_ALL_HEADERS)
	g++ -MMD -std=c++17 $(FL) $(VERSAT_FLAGS) -o $@ $(VERSAT_COMMON_FLAGS) $(CPP_OBJ) $(VERSAT_INCLUDE) $(VERSAT_LIBS) 

embedFile: $(BUILD_DIR)/templateData.hpp

calculateHash: $(BUILD_DIR)/calculateHash

versat: $(VERSAT_DIR)/versat $(BUILD_DIR)/calculateHash

type-info: $(TYPE_INFO_HDR) $(BUILD_DIR)/typeInfo.cpp

clean:
	-rm -fr build
	-rm -f *.a versat versat.d

.PHONY: versat

.SUFFIXES:

