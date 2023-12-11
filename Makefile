#CORE := iob_versat

#DISABLE_LINT:=1
#include submodules/LIB/setup.mk

VERSAT_DIR:=$(shell pwd)

# Default rule
all: versat

include $(VERSAT_DIR)/config.mk
include $(VERSAT_DIR)/sharedHardware.mk

versat-tools:
	$(MAKE) -C $(VERSAT_TOOLS_DIR) all

versat: versat-tools
	$(MAKE) -C $(VERSAT_COMP_DIR) versat

clean:
	-rm -fr build
	-rm -f *.a versat versat.d

.PHONY: pc-sim clean
