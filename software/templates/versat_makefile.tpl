#{set TRACE_TYPE "--trace"}
#{if arch.generateFSTFormat} #{set TRACE_TYPE "--trace-fst"} #{end}

TYPE_NAME := @{typename}

HARDWARE_SRC := #{join " " for file verilogFiles}@{file}#{end} @{hack}
#HARDWARE_SRC := $(wildcard @{generatedUnitsLocation}/*.v) @{hack}
HARDWARE_SRC += $(wildcard @{generatedUnitsLocation}/modules/*.v)

#{for source extraSources}
HARDWARE_SRC += $(wildcard @{source}/*.v) @{hack}
#{end}

VERILATOR_ROOT?=@{verilatorRoot}

INCLUDE := #{join " " for file includePaths}-I@{file}#{end} @{hack}

PID := $(shell cat /proc/$$$$/status | grep PPid | awk '{print $$2}')
JOBS := $(shell ps -p ${PID} -f | tail -n1 | grep -oP '\-j *\d+' | sed 's/-j//')
ifeq "${JOBS}" ""
JOBS := 1
endif

all: libaccel.a

# Joins wrapper with verilator object files into a library
libaccel.a: V@{typename}.h wrapper.o createVerilatorObjects
	ar -rcs libaccel.a wrapper.o $(wildcard ./obj_dir/*.o)

createVerilatorObjects: V@{typename}.h wrapper.o
	$(MAKE) verilatorObjects

#./obj_dir/V@{typename}_classes.mk: V@{typename}.h

# TODO: src folder should be set to absolute. Versat compiler knows the location 
V@{typename}.h: $(HARDWARE_SRC)
	+verilator -GAXI_ADDR_W=@{arch.addrSize} -GAXI_DATA_W=@{arch.dataSize} -GLEN_W=16 -CFLAGS -O2 --build -j $(JOBS) @{TRACE_TYPE} --cc $(HARDWARE_SRC) $(wildcard @{srcDir}/*.v) $(INCLUDE) --top-module $(TYPE_NAME)
	cp ./obj_dir/*.h ./

wrapper.o: V@{typename}.h wrapper.cpp
	g++ -std=c++17 -O2 -g -c -o wrapper.o -I $(VERILATOR_ROOT)/include $(abspath wrapper.cpp)

# Created after calling verilator. Need to recall make to have access to the variables 
-include ./obj_dir/V@{typename}_classes.mk

VERILATOR_SOURCE_DIR:=$(VERILATOR_ROOT)/include
ALL_VERILATOR_FILES:=$(VM_GLOBAL_FAST) $(VM_GLOBAL_SLOW)
ALL_VERILATOR_O:=$(patsubst %,./obj_dir/%.o,$(ALL_VERILATOR_FILES))
ALL_VERILATOR_CPPS:=$(patsubst %,$(VERILATOR_SOURCE_DIR)/%.cpp,$(ALL_VERILATOR_FILES))

./obj_dir/%.o: $(VERILATOR_SOURCE_DIR)/%.cpp
	g++ -g -w -c -o $@ $< -I$(VERILATOR_ROOT)/include

verilatorObjects: $(ALL_VERILATOR_O)
	echo $(ALL_VERILATOR_FILES)
	echo $(ALL_VERILATOR_CPPS)
	echo $(ALL_VERILATOR_O)
