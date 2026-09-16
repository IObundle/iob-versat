# SPDX-FileCopyrightText: 2025 IObundle
#
# SPDX-License-Identifier: MIT

# Basic info used by the document/ build (see py2hwsw's config_gen.py, which
# normally generates this file automatically for py2hwsw-managed cores).

NAME=versat
CSR_IF?=iob
BUILD_DIR_NAME=iob-versat
IS_FPGA=0

# Document build flags (see document/Makefile)
PYTHON_DIR?=.
ASICSYNTH?=0
FPGACOMP?=0
DOXYGEN?=0

CONFIG_BUILD_DIR = $(dir $(lastword $(MAKEFILE_LIST)))
ifneq ($(wildcard $(CONFIG_BUILD_DIR)/custom_config_build.mk),)
include $(CONFIG_BUILD_DIR)/custom_config_build.mk
endif
