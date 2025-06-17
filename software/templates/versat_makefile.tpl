TYPE_NAME := @{typeName}

VSIM_HEADER := @{simLoopHeader}
HARDWARE_FOLDER := @{hardwareFolder}
VHEADER := V$(TYPE_NAME).h
SOFTWARE_FOLDER := .
HARDWARE_SRC := @{hardwareUnits}

HARDWARE_SRC += $(wildcard $(HARDWARE_FOLDER)/modules/*.v)
VERILATOR_ROOT?=$(shell ./GetVerilatorRoot.sh)
INCLUDE := -I$(HARDWARE_FOLDER)

VERILATOR_COMMON_ARGS := --report-unoptflat -GLEN_W=20 -CFLAGS "-O2 -march=native" $(INCLUDE)
VERILATOR_COMMON_ARGS += -GAXI_ADDR_W=@{databusAddrSize}
VERILATOR_COMMON_ARGS += -GAXI_DATA_W=@{databusDataSize}
VERILATOR_COMMON_ARGS += @{traceType}

all: libaccel.a

# Joins wrapper with verilator object files into a library
libaccel.a: $(VHEADER) wrapper.o createVerilatorObjects $(VSIM_HEADER)
	ar -rcs libaccel.a wrapper.o $(wildcard ./obj_dir/*.o)

createVerilatorObjects: $(VHEADER) wrapper.o $(VSIM_HEADER)
	$(MAKE) verilatorObjects

VUnitWireInfo.h: $(VHEADER)
	./ExtractVerilatedSignals.py $(VHEADER) > VUnitWireInfo.h

$(VHEADER): $(HARDWARE_SRC)
	verilator $(VERILATOR_COMMON_ARGS) --cc $(HARDWARE_SRC) --top-module $(TYPE_NAME)
	$(MAKE) -C ./obj_dir -f V$(TYPE_NAME).mk
	cp ./obj_dir/*.h ./

VSuperAddress.h: $(HARDWARE_SRC)
	verilator $(VERILATOR_COMMON_ARGS) --cc $(HARDWARE_FOLDER)/SuperAddress.v --top-module SuperAddress
	$(MAKE) -C ./obj_dir -f VSuperAddress.mk
	cp ./obj_dir/*.h ./

wrapper.o: $(VHEADER) VUnitWireInfo.h wrapper.cpp $(VSIM_HEADER)
	g++ -std=c++17 -march=native -O2 -g -c -o wrapper.o -I $(VERILATOR_ROOT)/include $(abspath wrapper.cpp)

# Created after calling verilator. Need to recall make to have access to the variables
-include ./obj_dir/V$(TYPE_NAME)_classes.mk

VERILATOR_SOURCE_DIR:=$(VERILATOR_ROOT)/include
ALL_VERILATOR_FILES:=$(VM_GLOBAL_FAST) $(VM_GLOBAL_SLOW)
ALL_VERILATOR_O:=$(patsubst %,./obj_dir/%.o,$(ALL_VERILATOR_FILES))

./obj_dir/%.o: $(VERILATOR_SOURCE_DIR)/%.cpp
	g++ -w -march=native -O2 -c -o $@ $< -I$(VERILATOR_ROOT)/include

verilatorObjects: $(ALL_VERILATOR_O)
