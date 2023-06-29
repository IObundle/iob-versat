TYPE_NAME := @{typename}

HARDWARE_SRC := #{join " " for file verilogFiles}@{file}#{end} @{hack}
HARDWARE_SRC += #{join " " for file extraSources}@{file}#{end} @{hack}

INCLUDE := #{join " " for file includePaths}-I@{file}#{end} @{hack}

all: V$(TYPE_NAME).h $(VERILATOR_OBJ)

V$(TYPE_NAME).h: $(HARDWARE_SRC)
	verilator -GAXI_ADDR_W=64 --build --trace --cc $(HARDWARE_SRC) $(wildcard @{rootPath}/src/*.v) $(INCLUDE) --top-module $(TYPE_NAME)
	cp ./obj_dir/*.h ./
	cp ./obj_dir/*.o ./
	cp ./obj_dir/*.a ./libversat.a
