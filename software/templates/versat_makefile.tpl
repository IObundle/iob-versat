#{let TRACE_TYPE ""}
#{if traceEnabled} 
  #{set TRACE_TYPE "--trace"}
  #{if arch.generateFSTFormat} #{set TRACE_TYPE "--trace-fst"} #{end}
#{end}
TYPE_NAME := @{typeName}

HARDWARE_FOLDER := @{generatedUnitsLocation}
SOFTWARE_FOLDER := .

HARDWARE_SRC := #{for name allFilenames}$(HARDWARE_FOLDER)/@{name} #{end} 
HARDWARE_SRC += $(wildcard $(HARDWARE_FOLDER)/modules/*.v) # This can also be changed from wildcard to the relative path for each module. No point using wildcard when we have the entire information needed.

#{for source extraSources}
HARDWARE_SRC += $(wildcard @{source}/*.v)
#{end}

VERILATOR_ROOT?=$(shell ./GetVerilatorRoot.sh)

INCLUDE := -I$(HARDWARE_FOLDER)

all: libaccel.a

# Joins wrapper with verilator object files into a library
libaccel.a: V@{typeName}.h wrapper.o createVerilatorObjects #{if simulateLoops} VSuperAddress.h #{end}
	ar -rcs libaccel.a wrapper.o $(wildcard ./obj_dir/*.o)

createVerilatorObjects: V@{typeName}.h wrapper.o #{if simulateLoops} VSuperAddress.h #{end}
	$(MAKE) verilatorObjects

#./obj_dir/V@{typeName}_classes.mk: V@{typeName}.h

V@{typeName}.h: $(HARDWARE_SRC)
	verilator --report-unoptflat -GAXI_ADDR_W=@{arch.databusAddrSize} -GAXI_DATA_W=@{arch.databusDataSize} -GLEN_W=20 -CFLAGS "-O2 -march=native" @{TRACE_TYPE} --cc $(HARDWARE_SRC) $(INCLUDE) --top-module $(TYPE_NAME)
	$(MAKE) -C ./obj_dir -f V@{typeName}.mk
	cp ./obj_dir/*.h ./

#{if simulateLoops}
VSuperAddress.h: $(HARDWARE_SRC)
	verilator --report-unoptflat -GAXI_ADDR_W=@{arch.databusAddrSize} -GAXI_DATA_W=@{arch.databusDataSize} -GLEN_W=20 -CFLAGS "-O2 -march=native" @{TRACE_TYPE} --cc $(HARDWARE_FOLDER)/SuperAddress.v $(INCLUDE) --top-module SuperAddress
	$(MAKE) -C ./obj_dir -f VSuperAddress.mk
	cp ./obj_dir/*.h ./
#{end}

wrapper.o: V@{typeName}.h VSuperAddress.h wrapper.cpp  #{if simulateLoops} VSuperAddress.h #{end}
	g++ -std=c++17 -march=native -O2 -g -c -o wrapper.o -I $(VERILATOR_ROOT)/include $(abspath wrapper.cpp)

# Created after calling verilator. Need to recall make to have access to the variables 
-include ./obj_dir/V@{typeName}_classes.mk

VERILATOR_SOURCE_DIR:=$(VERILATOR_ROOT)/include
ALL_VERILATOR_FILES:=$(VM_GLOBAL_FAST) $(VM_GLOBAL_SLOW)
ALL_VERILATOR_O:=$(patsubst %,./obj_dir/%.o,$(ALL_VERILATOR_FILES))
ALL_VERILATOR_CPPS:=$(patsubst %,$(VERILATOR_SOURCE_DIR)/%.cpp,$(ALL_VERILATOR_FILES))

./obj_dir/%.o: $(VERILATOR_SOURCE_DIR)/%.cpp
	g++ -w -march=native -O2 -c -o $@ $< -I$(VERILATOR_ROOT)/include

verilatorObjects: $(ALL_VERILATOR_O)
#	@echo $(ALL_VERILATOR_FILES)
#	@echo $(ALL_VERILATOR_CPPS)
#	@echo $(ALL_VERILATOR_O)
