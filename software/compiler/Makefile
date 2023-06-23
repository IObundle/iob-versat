VERSAT_DIR:=$(abspath $(shell pwd)/../..)

#versat common parameters
include $(VERSAT_DIR)/core.mk
include $(VERSAT_TOOLS_DIR)/tools.mk

VERSAT_DEBUG:= 1

BUILD_DIR:=$(VERSAT_DIR)/build

VERSAT_LIBS:=-lstdc++ -lm -lgcc -lc -pthread -ldl #
#pc headers
VERSAT_HDR+=$(wildcard $(VERSAT_PC_DIR)/*.hpp)
VERSAT_HDR+=$(wildcard $(VERSAT_COMPILER_DIR)/*.hpp)

VERILATE_FLAGS := -g -fPIC

VERSAT_INCLUDE += -I$(VERSAT_PC_DIR) -I$(VERSAT_COMPILER_DIR) -I$(BUILD_DIR)/ -I$(VERSAT_COMMON_DIR)
VERSAT_DEFINE += -DPC

CPP_FILES := $(wildcard $(VERSAT_COMPILER_DIR)/*.cpp)
VERSAT_COMPILER_OBJ := $(patsubst $(VERSAT_COMPILER_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_FILES))
CPP_OBJ := $(VERSAT_COMPILER_OBJ)

# Still need to check, because other people do not want to build the compiler with debug code
ifeq ($(VERSAT_DEBUG),1)
VERSAT_LIBS+= -lSDL2  -lGL -ldl
VERSAT_INCLUDE+= -I$(VERSAT_COMPILER_DIR)/IMNODES -I$(VERSAT_COMPILER_DIR)/IMGUI -I$(VERSAT_COMPILER_DIR)/IMGUI/backends -I/usr/include/SDL2

IMGUI_SRC := $(wildcard $(VERSAT_COMPILER_DIR)/IMGUI/*.cpp)
IMGUI_OBJ := $(patsubst $(VERSAT_COMPILER_DIR)/IMGUI/%.cpp,$(BUILD_DIR)/%.o,$(IMGUI_SRC))

IMGUI_BACKEND_SRC := $(VERSAT_COMPILER_DIR)/IMGUI/backends/imgui_impl_opengl3.cpp $(VERSAT_COMPILER_DIR)/IMGUI/backends/imgui_impl_sdl.cpp
IMGUI_BACKEND_OBJ := $(patsubst $(VERSAT_COMPILER_DIR)/IMGUI/backends/%.cpp,$(BUILD_DIR)/%.o,$(IMGUI_BACKEND_SRC))

IMNODES_SRC := $(wildcard $(VERSAT_COMPILER_DIR)/IMNODES/*.cpp)
IMNODES_OBJ := $(patsubst $(VERSAT_COMPILER_DIR)/IMNODES/%.cpp,$(BUILD_DIR)/%.o,$(IMNODES_SRC))

CPP_FILES += $(IMGUI_SRC) $(IMGUI_BACKEND_SRC) $(IMNODES_SRC)
CPP_OBJ += $(IMGUI_OBJ) $(IMGUI_BACKEND_OBJ) $(IMNODES_OBJ)
endif

CPP_OBJ += $(VERSAT_COMMON_OBJ)
CPP_OBJ += $(VERSAT_TEMPLATES_OBJ)
CPP_OBJ += $(BUILD_DIR)/typeInfo.o

CPP_OBJ_WITHOUT_COMPILER:=$(filter-out $(BUILD_DIR)/versatCompiler.o,$(CPP_OBJ))

$(VERSAT_DIR)/versat: $(CPP_OBJ)
	g++ -MMD -MP -std=c++17 -DVERSAT_DEBUG -o $@ -g $(CPP_OBJ) $(VERSAT_INCLUDE) $(VERSAT_LIBS)

$(VERSAT_DIR)/libversat.a: $(CPP_OBJ_WITHOUT_COMPILER)
	ar rcs $(VERSAT_DIR)/libversat.a $(CPP_OBJ_WITHOUT_COMPILER)

versat: $(VERSAT_DIR)/versat $(VERSAT_DIR)/libversat.a

VERSAT_COMPILER_DEPENDS := $(patsubst %.o,%.d,$(VERSAT_COMPILER_OBJ))
-include  $(VERSAT_COMPILER_DEPENDS)

$(BUILD_DIR)/%.o : $(VERSAT_COMPILER_DIR)/%.cpp
	g++ -MMD -MP -std=c++17 -DVERSAT_DEBUG -g -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

ifeq ($(VERSAT_DEBUG),1)
$(BUILD_DIR)/%.o: $(VERSAT_COMPILER_DIR)/IMGUI/%.cpp
	g++ -c -o $@ -D_REENTRANT $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o: $(VERSAT_COMPILER_DIR)/IMGUI/backends/%.cpp
	g++ -c -o $@ -D_REENTRANT $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/%.o: $(VERSAT_COMPILER_DIR)/IMNODES/%.cpp
	g++ -c -o $@ -D_REENTRANT $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)
endif

$(shell mkdir -p $(BUILD_DIR))

verilator_test: $(CPP_OBJ)

.PHONY: versat

#.NOTPARALLEL: $(BUILD_DIR)/V%.h

.SUFFIXES:


