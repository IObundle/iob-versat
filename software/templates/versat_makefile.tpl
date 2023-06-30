TYPE_NAME := @{typename}

HARDWARE_SRC := #{join " " for file verilogFiles}@{file}#{end} @{hack}
HARDWARE_SRC += #{join " " for file extraSources}@{file}#{end} @{hack}

VERILATOR_ROOT?=@{verilatorRoot}
VERSAT_DIR?=@{versatDir}

INCLUDE := #{join " " for file includePaths}-I@{file}#{end} @{hack}

all: libaccel.a

libaccel.a: V$(TYPE_NAME).h wrapper.o
	ar -rcs libaccel.a wrapper.o $(wildcard ./obj_dir/*.o)   
	#cp ./obj_dir/*.o ./
	#cp ./obj_dir/*.a ./libaccel.a
   #ar -racs SMVM__ALL.o libaccel.a wrapper.o 
	#ar -rbs libaccel.a $(VERSAT_DIR)/libaccel.a 

# TODO: src folder should be set to absolute. Versat compiler knows the location 
V$(TYPE_NAME).h: $(HARDWARE_SRC)
	verilator -GAXI_ADDR_W=64 --build --trace --cc $(HARDWARE_SRC) $(wildcard @{srcDir}/*.v) $(INCLUDE) --top-module $(TYPE_NAME)
	cp ./obj_dir/*.h ./

wrapper.o: V$(TYPE_NAME).h wrapper.cpp
	g++ -g -c -o wrapper.o -I $(VERILATOR_ROOT)/include -I $(VERSAT_DIR)/software/compiler -I $(VERSAT_DIR)/software/common wrapper.cpp 

