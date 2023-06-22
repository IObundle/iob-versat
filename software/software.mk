include $(VERSAT_DIR)/core.mk

#include
INCLUDE+=-I$(VERSAT_SW_DIR)

#headers
VERSAT_HDR += $(wildcard $(VERSAT_SW_DIR)/*.hpp)

PHONY: clean
