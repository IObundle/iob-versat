SCRIPTS_DIR=../../../scripts
FU_DIR=../../src
TBS=$(wildcard *_tb.v)
F_UNITS=$(foreach tb,$(TBS),$(subst _tb.v,,$(tb)))
COV_RPTS=$(foreach fu,$(F_UNITS),$(addsuffix _coverage.rpt ,$(fu)))

FU?=Relu
VCD?=0

VERILATED_PATH=./obj_dir
VSRC=$(wildcard $(FU_DIR)/*.v)

all coverage-all-fus: debug $(COV_RPTS)

coverage: $(FU)_coverage.rpt

COV_FLAGS= -Wall -cc
# alias for --main --exe --build --timing
# --main: Generate a main() function to run the simulation.
# --exe: Generate an executable simulation program.
# --build: Compile and link the generated C++ code into an executable.
# --timing: Support delays, etc
COV_FLAGS+= --binary
COV_FLAGS+= --trace
COV_FLAGS+= --coverage
ifeq ($(VCD),1)
COV_FLAGS+=-DVCD
endif
COV_FLAGS+=-I$(FU_DIR)
COV_FLAGS+=-Wno-lint

$(VERILATED_PATH)/V%_tb: %_tb.v
	verilator $(COV_FLAGS) --top-module $*_tb $< $(VSRC)

MERGED_DAT=merged.dat
%_coverage.rpt: $(VERILATED_PATH)/V%_tb
	@echo "Generating $@"
	# Run simulation
	./$< +verilator+coverage+file+$*_cov.dat
	# merge coverage
	verilator_coverage --write $(MERGED_DAT) $*_cov.dat
	# annotate coverage
	verilator_coverage --annotate $*_cov_annotated --annotate-min 2 --annotate-all $(MERGED_DAT)
	./$(SCRIPTS_DIR)/iob_cov_analyze.py ./$*_cov_annotated -o $@ -E $*_tb.v

clean:
	rm -rf $(VERILATED_PATH) *.vcd
	rm -rf *_coverage.rpt $(MERGED_DAT) *_cov_annotated \
		coverage.info coverage_html

debug:
	@echo "Coverage Simulation"
	@echo "TBS: $(TBS)"
	@echo "F_UNITS: $(F_UNITS)"
	@echo "COV_RPTS: $(COV_RPTS)"

.PHONY: all clean coverage coverage-all-fus
