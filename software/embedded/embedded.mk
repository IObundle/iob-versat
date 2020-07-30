#versat common parameters
include $(VERSAT_DIR)/software/software.mk

#include
INCLUDE+=-I$(VERSAT_SW_DIR)/embedded/

#header
HDR+=$(wildcard $(VERSAT_SW_DIR)/embedded/*.hpp)
