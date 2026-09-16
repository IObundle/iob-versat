# SPDX-FileCopyrightText: 2025 IObundle
#
# SPDX-License-Identifier: MIT

SHELL := bash

CORE := iob_versat
BOARD ?= iob_basys3

BUILD_DIR ?= $(shell nix-shell --run "py2hwsw $(CORE) print_build_dir")

setup:
	nix-shell --run "py2hwsw $(CORE) setup --no_verilog_lint"

fpga-build:
	nix-shell --run "make clean setup && make -C $(BUILD_DIR) fpga-build FPGA_TOP=$(CORE) BOARD=$(BOARD)"

clean:
	nix-shell --run "py2hwsw $(CORE) clean --build_dir '$(BUILD_DIR)'"
	@find . -name \*~ -delete

.PHONY: setup fpga-build clean
