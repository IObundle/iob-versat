SHELL := /bin/bash

VERSAT_DIR:=$(shell pwd)

# Default rule
all: versat

include $(VERSAT_DIR)/core.mk
include $(VERSAT_DIR)/sharedHardware.mk

libverilator.a: tools
	$(MAKE) -C $(VERSAT_COMP_DIR) libversat

tools:
	$(MAKE) -C $(VERSAT_TOOLS_DIR) all

versat: tools
	$(MAKE) -C $(VERSAT_COMP_DIR) versat

clean:
	-rm -fr build
	-rm -f *.a versat versat.d

.PHONY: pc-sim clean
