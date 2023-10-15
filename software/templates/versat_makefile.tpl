#{set TRACE_TYPE "--trace"}
#{if arch.generateFSTFormat} #{set TRACE_TYPE "--trace-fst"} #{end}

TYPE_NAME := @{typename}

HARDWARE_SRC := #{join " " for file verilogFiles}@{file}#{end} @{hack}
HARDWARE_SRC += #{join " " for file extraSources}@{file}#{end} @{hack}

VERILATOR_ROOT?=@{verilatorRoot}
VERSAT_DIR?=@{versatDir}

INCLUDE := #{join " " for file includePaths}-I@{file}#{end} @{hack}

PID := $(shell cat /proc/$$$$/status | grep PPid | awk '{print $$2}')
JOBS := $(shell ps -p ${PID} -f | tail -n1 | grep -oP '\-j *\d+' | sed 's/-j//')
ifeq "${JOBS}" ""
JOBS := 1
endif

all: libaccel.a

libaccel.a: V$(TYPE_NAME).h wrapper.o
	ar -rcs libaccel.a wrapper.o $(wildcard ./obj_dir/*.o)

# TODO: src folder should be set to absolute. Versat compiler knows the location 
V$(TYPE_NAME).h: $(HARDWARE_SRC)
	verilator -GAXI_ADDR_W=@{arch.addrSize} -GAXI_DATA_W=@{arch.dataSize} -GLEN_W=16 -CFLAGS -O2 --build -j $(JOBS) @{TRACE_TYPE} --cc $(HARDWARE_SRC) $(wildcard @{srcDir}/*.v) $(INCLUDE) --top-module $(TYPE_NAME)
	cp ./obj_dir/*.h ./

wrapper.o: V$(TYPE_NAME).h wrapper.cpp
	g++ -std=c++17 -O2 -g -c -o wrapper.o -I $(VERILATOR_ROOT)/include -I $(VERSAT_DIR)/software/compiler -I $(VERSAT_DIR)/software/common wrapper.cpp 

