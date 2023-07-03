#versat common parameters
include $(VERSAT_DIR)/software/software.mk
include $(VERSAT_DIR)/sharedHardware.mk

#pc sources
SRC+=$(VERSAT_SW_DIR)/embedded/versat.c
SRC+=$(VERSAT_SW_DIR)/embedded/memory.c
#SRC+=$(VERSAT_COMMON_DIR)/utilsCommon.c

BUILD_DIR :=./build

CFLAGS+=-DSIM

# includes
INCLUDE+= -I$(VERSAT_SW_DIR)/embedded/ -I$(VERSAT_COMMON_DIR)/ -I$(BUILD_DIR)/ 