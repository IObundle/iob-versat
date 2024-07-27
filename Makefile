VERSAT_DIR:=$(shell pwd)

# Default rule
all: versat

include $(VERSAT_DIR)/config.mk

versat-tools:
	$(MAKE) -C $(VERSAT_TOOLS_DIR) all

versat: versat-tools
	$(MAKE) -C $(VERSAT_COMP_DIR) versat

type-info:
	$(MAKE) -C $(VERSAT_TOOLS_DIR) type-info

clean:
	-rm -fr build
	-rm -f *.a versat versat.d

.PHONY: pc-sim clean
