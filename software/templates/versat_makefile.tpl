TYPE_NAME := @{typename}

HARDWARE_SRC := #{join " " for file verilogFiles}@{file}#{end}

@{hack}

VERILATOR_ROOT := /usr/local/share/verilator

VERILATOR_SRC := $(VERILATOR_ROOT)/include/verilated.cpp $(VERILATOR_ROOT)/include/verilated_vcd_c.cpp #$(VERILATOR_ROOT)/include/verilated_threads.cpp 
VERILATOR_OBJ := $(patsubst $(VERILATOR_ROOT)/include/%.cpp,%.o,$(VERILATOR_SRC))

all: V$(TYPE_NAME).h $(VERILATOR_OBJ)

V$(TYPE_NAME).h: $(HARDWARE_SRC)
	verilator --cc $(HARDWARE_SRC) $(wildcard ../src/*.v) --top-module $(TYPE_NAME)
	$(MAKE) -C obj_dir -f V$(TYPE_NAME).mk
	cp ./obj_dir/*.h ./
	cp ./obj_dir/*.o ./

%.o: $(VERILATOR_ROOT)/include/%.cpp
	-g++ -g -w -c -o $@ $< -I$(VERILATOR_ROOT)/include
