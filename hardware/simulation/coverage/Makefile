FU_DIR=../../src
TBS=$(wildcard *_tb.v)
F_UNITS=$(foreach tb,$(TBS),$(subst _tb.v,,$(tb)))
COV_RPTS=$(foreach fu,$(F_UNITS),$(addsuffix _coverage.rpt ,$(fu)))

FU?=Relu

VSRC=$(wildcard $(FU_DIR)/*.v)

all coverage-all-fus: debug $(COV_RPTS)

coverage: $(FU)_coverage.rpt

%_coverage.rpt: %_tb.v
	@echo "Generating $@"
	# TODO:
	# 1. Verilate sources for coverage
	# 2. Run simulation with coverage enabled
	# 3. Generate coverage report
	# 4. Generate coverage report

debug:
	@echo "Coverage Simulation"
	@echo "TBS: $(TBS)"
	@echo "F_UNITS: $(F_UNITS)"
	@echo "COV_RPTS: $(COV_RPTS)"
