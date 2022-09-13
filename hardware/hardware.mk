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

#included files
INCLUDE+=$(incdir). $(incdir)$(AXI_DIR)/hardware/include

VHDR+=$(wildcard $(VERSAT_HW_DIR)/include/*.vh)

VHDR+=m_versat_axi_m_port.vh
VHDR+=m_versat_axi_m_write_port.vh
VHDR+=m_versat_axi_m_read_port.vh
VHDR+=m_m_versat_axi_write_portmap.vh
VHDR+=m_m_versat_axi_read_portmap.vh

#sources
VSRC+=$(wildcard $(VERSAT_HW_DIR)/src/*.v)
VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/tdp_ram/iob_tdp_ram.v # used by xmem
VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/2p_ram/iob_2p_ram.v # used by vread and vwrite
VSRC+=$(VERSAT_DIR)/submodules/MEM/ram/dp_ram/iob_dp_ram.v # Lookup table

m_versat_axi_m_port.vh:
	$(LIB_DIR)/software/python/axi_gen.py axi_m_port 'm_versat_' 'm_'

m_versat_axi_m_write_port.vh:
	$(LIB_DIR)/software/python/axi_gen.py axi_m_write_port 'm_versat_' 'm_'

m_versat_axi_m_read_port.vh:
	$(LIB_DIR)/software/python/axi_gen.py axi_m_read_port 'm_versat_' 'm_'

m_m_versat_axi_write_portmap.vh:
	$(LIB_DIR)/software/python/axi_gen.py axi_write_portmap 'm_versat_' 'm_' 'm_'

m_m_versat_axi_read_portmap.vh:
	$(LIB_DIR)/software/python/axi_gen.py axi_read_portmap 'm_versat_' 'm_' 'm_'