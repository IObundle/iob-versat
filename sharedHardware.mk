SHARED_UNITS_PATH := $(VERSAT_DIR)/hardware/src

VERILATE_UNIT_FILES := $(wildcard $(SHARED_UNITS_PATH)/*.v)
VERILATE_UNIT_BASIC := $(patsubst $(SHARED_UNITS_PATH)/%.v,%,$(VERILATE_UNIT_FILES))

V_INCLUDE+=$(incdir)$(VERSAT_DIR)/submodules/MEM/hardware/ram/tdp_ram
V_INCLUDE+=$(incdir)$(VERSAT_DIR)/submodules/MEM/hardware/ram/2p_ram
V_INCLUDE+=$(incdir)$(VERSAT_DIR)/submodules/MEM/hardware/ram/dp_ram
V_INCLUDE+=$(incdir)$(VERSAT_DIR)/submodules/MEM/hardware/fifo/sfifo
V_INCLUDE+=$(incdir)$(VERSAT_DIR)/submodules/MEM/hardware/fifo
V_INCLUDE+=$(incdir)$(VERSAT_DIR)/submodules/FPU/hardware/include
V_INCLUDE+=$(incdir)$(VERSAT_DIR)/submodules/FPU/hardware/src
V_INCLUDE+=$(incdir)$(VERSAT_DIR)/submodules/FPU/submodules/DIV/hardware/src
V_INCLUDE+=$(incdir)$(VERSAT_DIR)/hardware/src/units
V_INCLUDE+=$(incdir)$(VERSAT_DIR)/hardware/src

VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/ram/tdp_ram/iob_tdp_ram.v # used by xmem
VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/ram/2p_ram/iob_2p_ram.v # used by vread and vwrite
VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/ram/2p_asym_ram/iob_2p_asym_ram.v # used by vread and vwrite
VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/ram/dp_ram/iob_dp_ram.v # Lookup table
VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/fifo/sfifo/iob_sync_fifo.v
VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/fifo/bin_counter.v
VSRC+=$(VERSAT_DIR)/submodules/FPU/submodules/DIV/hardware/src/div_subshift.v
#VSRC+=$(VERSAT_DIR)/hardware/src/MyAddressGen.v 
#VSRC+=$(VERSAT_DIR)/hardware/src/xaddrgen.v 
#VSRC+=$(VERSAT_DIR)/hardware/src/xaddrgen2.v 
#VSRC+=$(VERSAT_DIR)/hardware/src/MemoryWriter.v
#VSRC+=$(VERSAT_DIR)/hardware/src/MemoryReader.v
VSRC+=$(wildcard $(VERSAT_DIR)/submodules/FPU/hardware/src/*.v)

VSRC+=$(VERILATE_UNIT_FILES) # Required by Icarus
