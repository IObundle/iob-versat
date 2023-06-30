TYPE_NAME := @{typename}

HARDWARE_SRC := #{join " " for file verilogFiles}@{file}#{end} @{hack}
HARDWARE_SRC += #{join " " for file extraSources}@{file}#{end} @{hack}

VERILATOR_ROOT?=@{verilatorRoot}
VERSAT_DIR?=@{versatDir}

INCLUDE := #{join " " for file includePaths}-I@{file}#{end} @{hack}

all: libversat.a

libversat.a: V$(TYPE_NAME).h wrapper.o
	#cp ./obj_dir/*.h ./
	#cp ./obj_dir/*.o ./
	cp ./obj_dir/*.a ./libversat.a
	#ar -racs SMVM__ALL.o libversat.a wrapper.o 
	#ar -rbs libversat.a $(VERSAT_DIR)/libversat.a 

V$(TYPE_NAME).h: $(HARDWARE_SRC)
	verilator -GAXI_ADDR_W=64 --build --trace --cc $(HARDWARE_SRC) $(wildcard /home/zettasticks/IOBundle/iob-soc-sha/software/pc-emul/src/*.v) $(INCLUDE) --top-module $(TYPE_NAME)

wrapper.o: wrapper.cpp
	g++ -g -c -o wrapper.o -I $(VERILATOR_ROOT)/include -I $(VERSAT_DIR)/software/compiler -I $(VERSAT_DIR)/software/common wrapper.cpp 

