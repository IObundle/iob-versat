VERSAT_DIR:=.
include core.mk

pc-emul:
	make -C $(VERSAT_SW_DIR)

clean:
	-make -C $(VERSAT_SW_DIR) clean

.PHONY: pc-sim clean
