include $(VERSAT_DIR)/core.mk

USE_NETLIST ?=0

include $(AXI_DIR)/config.mk

#SUBMODULE HARDWARE

#lib
ifneq (LIB,$(filter LIB, $(SUBMODULES)))
SUBMODULES+=LIB
INCLUDE+=$(incdir)$(LIB_DIR)/hardware/include
VHDR+=$(wildcard $(LIB_DIR)/hardware/include/*.vh)
endif

#VERSAT HARDWARE
#hardware include dirs
INCLUDE+=$(incdir)$(VERSAT_HW_DIR)/include
INCLUDE+=$(incdir)$(VERSAT_DIR)/submodules/LIB/hardware/include

VSRC+=$(VERSAT_DIR)/submodules/LIB/hardware/iob2axi/iob2axi.v
VSRC+=$(VERSAT_DIR)/submodules/LIB/hardware/iob2axi/iob2axi_rd.v
VSRC+=$(VERSAT_DIR)/submodules/LIB/hardware/iob2axi/iob2axi_wr.v

#included files
INCLUDE+=$(incdir). $(incdir)$(AXI_DIR)/hardware/include

VHDR+=$(wildcard $(VERSAT_HW_DIR)/include/*.vh)

VHDR+=m_axi_m_port.vh
VHDR+=m_axi_write_m_port.vh
VHDR+=m_axi_read_m_port.vh
VHDR+=m_m_axi_write_portmap.vh
VHDR+=m_m_axi_read_portmap.vh

#sources
VSRC+=$(wildcard $(VERSAT_HW_DIR)/src/*.v)
VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/tdp_ram/iob_tdp_ram.v # used by xmem
VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/2p_ram/iob_2p_ram.v # used by vread and vwrite
VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/dp_ram/iob_dp_ram.v # Lookup table
#VSRC+=$(VERSAT_DIR)/submodules/DMA/hardware/src/dma_axi.v
#VSRC+=$(VERSAT_DIR)/submodules/DMA/hardware/src/dma_axi_r.v
#VSRC+=$(VERSAT_DIR)/submodules/DMA/hardware/src/dma_axi_w.v

m_axi_m_port.vh:
	$(AXI_GEN) axi_m_port AXI_ADDR_W AXI_DATA_W 'm_'

m_axi_write_m_port.vh:
	$(AXI_GEN) axi_write_m_port AXI_ADDR_W AXI_DATA_W 'm_'

m_axi_read_m_port.vh:
	$(AXI_GEN) axi_read_m_port AXI_ADDR_W AXI_DATA_W 'm_'

m_m_axi_write_portmap.vh:
	$(AXI_GEN) axi_write_portmap AXI_ADDR_W AXI_DATA_W 'm_' 'm_'

m_m_axi_read_portmap.vh:
	$(AXI_GEN) axi_read_portmap AXI_ADDR_W AXI_DATA_W 'm_' 'm_'