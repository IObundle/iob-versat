#versat common parameters
include $(VERSAT_DIR)/software/software.mk

#pc sources
SRC+=$(VERSAT_SW_DIR)/embedded/versat.cpp
SRC+=$(VERSAT_SW_DIR)/embedded/memory.cpp
SRC+=$(VERSAT_DIR)/software/utilsCommon.cpp

# includes
INCLUDE+= -I$(VERSAT_SW_DIR)/embedded/