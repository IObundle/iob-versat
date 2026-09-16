# SPDX-FileCopyrightText: 2025 IObundle
#
# SPDX-License-Identifier: MIT

# Makefile segment included by document/Makefile (see py2hwsw's
# iob_system/document/doc_build.mk for the reference pattern this follows).

# Include the Implementation Results section in the DS. It only actually
# renders once FPGA/ASIC synthesis output files (quartus.tex, vivado.tex,
# asic.tex) are present next to this Makefile, since those are what the
# Makefile uses to detect INTEL_FPGA/AMD_FPGA/UMC130_ASIC.
RESULTS ?= 1
