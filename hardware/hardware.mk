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

#hardware include dirs
INCLUDE+=$(incdir) $(VERSAT_HW_DIR)/include

#UART HARDWARE
#included files
#VHDR+=$(wildcard $(VERSAT_HW_DIR)/include/*.vh)
#sources
VSRC+=$(wildcard $(VERSAT_HW_DIR)/src/*.v)


#cpu accessible registers
#$(UART_HW_DIR)/include/UARTsw_reg_gen.v $(UART_HW_DIR)/include/UARTsw_reg.vh: $(UART_HW_DIR)/include/UARTsw_reg.v
#	$(LIB_DIR)/software/mkregs.py $< HW
#	mv UARTsw_reg_gen.v $(UART_HW_DIR)/include
#	mv UARTsw_reg.vh $(UART_HW_DIR)/include
