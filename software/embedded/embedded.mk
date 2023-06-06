#versat common parameters
include $(VERSAT_DIR)/software/software.mk

#pc sources
SRC+=$(VERSAT_SW_DIR)/embedded/versat.cpp
SRC+=$(VERSAT_SW_DIR)/embedded/memory.cpp
SRC+=$(VERSAT_COMMON_DIR)/utilsCommon.cpp

BUILD_DIR :=./build

CFLAGS+=-DSIM

# includes
INCLUDE+= -I$(VERSAT_SW_DIR)/embedded/ -I$(VERSAT_COMMON_DIR)/ -I$(BUILD_DIR)/ 