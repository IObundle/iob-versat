include $(VERSAT_DIR)/core.mk

#include
INCLUDE+=-I$(VERSAT_SW_DIR)

#headers
HDR+=$(VERSAT_SW_DIR)/*.h

#sources
#SRC+=$(VERSAT_SW_DIR)/iob-uart.c