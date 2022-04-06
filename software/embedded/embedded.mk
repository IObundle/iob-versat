#versat common parameters
include $(VERSAT_DIR)/software/software.mk

#pc sources
SRC+=$(VERSAT_SW_DIR)/embedded/versat.c
SRC+=$(VERSAT_SW_DIR)/embedded/unitVerilogWrappers.c
SRC+=$(VERSAT_SW_DIR)/versatCommon.c