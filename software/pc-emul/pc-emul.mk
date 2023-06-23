include $(VERSAT_DIR)/core.mk
include $(VERSAT_DIR)/sharedHardware.mk

SRC+=$(VERSAT_PC_DIR)/versat.cpp

VERSAT_INCLUDE:= -I $(VERSAT_COMPILER_DIR) -I $(VERSAT_COMMON_DIR) -I $(VERSAT_SW_DIR) -I $(VERILATOR_INCLUDE) 

#OBJ+=./build/versat.o

#./build/versat.o: $(VERSAT_PC_DIR)/versat.cpp ./generated/accel.hpp
#	g++ $(GLOBAL_CFLAGS) -c -o $@ $(VERSAT_PC_DIR)/versat.cpp $(VERSAT_INCLUDE) $(VERSAT_SPEC_INCLUDE)

