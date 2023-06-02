TYPE_NAME := @{typename}

HARDWARE_SRC := #{join " " for file verilogFiles}@{file}#{end}

@{hack}

V$(TYPE_NAME).h: $(HARDWARE_SRC)
	verilator --cc $(HARDWARE_SRC) $(wildcard ../src/*.v) --top-module $(TYPE_NAME)
	$(MAKE) -C obj_dir -f V$(TYPE_NAME).mk
	cp ./obj_dir/*.h ./
	cp ./obj_dir/*.o ./

all: V$(TYPE_NAME).h
