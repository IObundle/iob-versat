VERSAT_DIR:=.
include core.mk
include sharedHardware.mk

HARDWARE := $(patsubst %,./hardware/src/%.v,$(VERILATE_UNIT_BASIC))

compiler:
	$(MAKE) -C $(VERSAT_PC_EMUL) compiler

run:
	./versatCompiler -I /home/zettasticks/IOBundle/iob-soc-sha/submodules/VERSAT/hardware/include/ $(HARDWARE) testVersatSpecification.txt -s -T CH

clean:
	$(MAKE) -C $(VERSAT_SW_DIR) clean

.PHONY: pc-sim clean
