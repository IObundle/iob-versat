include $(VERSAT_DIR)/core.mk

USE_NETLIST ?=0

#SUBMODULE HARDWARE
#intercon
ifneq (INTERCON,$(filter INTERCON, $(SUBMODULES)))
SUBMODULES+=INTERCON
include $(INTERCON_DIR)/hardware/hardware.mk
endif

#lib
ifneq (LIB,$(filter LIB, $(SUBMODULES)))
SUBMODULES+=LIB
INCLUDE+=$(incdir) $(LIB_DIR)/hardware/include
VHDR+=$(wildcard $(LIB_DIR)/hardware/include/*.vh)
endif

#VERSAT HARDWARE
#hardware include dirs
INCLUDE+=$(incdir) $(VERSAT_HW_DIR)/include

#included files
VHDR+=$(wildcard $(VERSAT_HW_DIR)/include/*.vh)

#sources
VSRC+=$(wildcard $(VERSAT_HW_DIR)/src/*.v)
VSRC+=$(VERSAT_DIR)/submodules/RAM/ram/2p_ram/iob_2p_ram.v # used by vread and vwrite
VSRC+=$(VERSAT_DIR)/submodules/DMA/hardware/src/dma_axi.v
VSRC+=$(VERSAT_DIR)/submodules/DMA/hardware/src/dma_axi_r.v
VSRC+=$(VERSAT_DIR)/submodules/DMA/hardware/src/dma_axi_w.v
