# SPDX-FileCopyrightText: 2025 IObundle
#
# SPDX-License-Identifier: MIT

# Builds Versat's generated PC emulation of SimpleExample and runs main.cpp on it.
# Generate the accelerator first, from the repository root:
#   ./versat examples/simple_example/simple_example.versat -t SimpleExample

VERSAT_OUTPUT ?= ../../versatOutput
SW_DIR := $(VERSAT_OUTPUT)/software

run: simple_example
	./simple_example

simple_example: main.cpp $(SW_DIR)/libaccel.a
	g++ -std=c++17 -O2 -I$(SW_DIR) main.cpp $(SW_DIR)/libaccel.a -lm -lstdc++ -lpthread -latomic -o $@

# VerilatorMake.mk re-invokes $(MAKE) without -f, so it must be included from a Makefile in its own directory.
$(SW_DIR)/Makefile:
	echo "include VerilatorMake.mk" > $@

$(SW_DIR)/libaccel.a: $(SW_DIR)/Makefile
	$(MAKE) -C $(SW_DIR) libaccel.a

clean:
	rm -f simple_example system.vcd

.PHONY: run clean
