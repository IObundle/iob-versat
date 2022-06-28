include $(VERSAT_DIR)/core.mk

USE_NETLIST ?=0

#SUBMODULE HARDWARE

#lib
ifneq (LIB,$(filter LIB, $(SUBMODULES)))
SUBMODULES+=LIB
INCLUDE+=$(incdir) $(LIB_DIR)/hardware/include
VHDR+=$(wildcard $(LIB_DIR)/hardware/include/*.vh)
endif

#VERSAT HARDWARE
#hardware include dirs
INCLUDE+=$(incdir) $(VERSAT_HW_DIR)/include
INCLUDE+=$(incdir) $(VERSAT_DIR)/submodules/LIB/hardware/include

#included files
VHDR+=$(wildcard $(VERSAT_HW_DIR)/include/*.vh)

VHDR+=cpu_axi4_m_if.vh

#sources
VSRC+=$(wildcard $(VERSAT_HW_DIR)/src/*.v)
VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/tdp_ram/iob_tdp_ram.v # used by xmem
VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/2p_ram/iob_2p_ram.v # used by vread and vwrite
VSRC+=$(VERSAT_DIR)/submodules/DMA/hardware/src/dma_axi.v
VSRC+=$(VERSAT_DIR)/submodules/DMA/hardware/src/dma_axi_r.v
VSRC+=$(VERSAT_DIR)/submodules/DMA/hardware/src/dma_axi_w.v

cpu_axi4_m_if.vh: 
	$(VERSAT_DIR)/submodules/LIB/software/python/axi_gen.py axi_m_port cpu_ m_