VERSAT_DIR:=.

# Default rule
all: versat

include $(VERSAT_DIR)/core.mk
include $(VERSAT_DIR)/sharedHardware.mk

#HARDWARE := $(patsubst %,./hardware/src/%.v,$(VERILATE_UNIT_BASIC))

tools:
	$(MAKE) -C $(VERSAT_TOOLS_DIR) all

versat: tools
	$(MAKE) -C $(VERSAT_COMP_DIR) versat

clean:
	-rm -fr build
	-rm -f *.a versat versat.d

.PHONY: pc-sim clean
