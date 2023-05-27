VERSAT_DIR:=.
include core.mk

pc-emul:
	$(MAKE) -C $(VERSAT_SW_DIR)

clean:
	$(MAKE) -C $(VERSAT_SW_DIR) clean

.PHONY: pc-sim clean
