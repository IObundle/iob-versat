#versat common parameters
include $(VERSAT_DIR)/software/software.mk

#pc sources
SRC+=$(VERSAT_SW_DIR)/embedded/versat.cpp

# includes
INCLUDE+= -I$(VERSAT_SW_DIR)/embedded/