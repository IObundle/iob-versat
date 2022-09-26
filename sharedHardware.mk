V_INCLUDE+=-I$(VERSAT_DIR)/submodules/MEM/ram/tdp_ram
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/MEM/ram/2p_ram
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/MEM/ram/dp_ram
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/MEM/fifo/sfifo
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/MEM/fifo

VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/tdp_ram/iob_tdp_ram.v # used by xmem
VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/2p_ram/iob_2p_ram.v # used by vread and vwrite
VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/dp_ram/iob_dp_ram.v # Lookup table
VSRC+=$(VERSAT_DIR)/submodules/MEM/fifo/sfifo/iob_sync_fifo.v
VSRC+=$(VERSAT_DIR)/submodules/MEM/fifo/bin_counter.v