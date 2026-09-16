# SPDX-FileCopyrightText: 2025 IObundle
#
# SPDX-License-Identifier: MIT

# Wraps a Versat-generated accelerator as a standalone py2hwsw core, so it
# can be set up, linted, and built for FPGA the same way as any other core.
#
# Unlike a regular py2hwsw core, this one does not describe its own
# hardware: it calls the `versat` compiler to generate it. That is the
# only part of the old, IOb-SoC-only CreateVersatClass (see iob_versat.py
# at the repository root) this wrapper reuses; everything else here is
# py2hwsw's own core interface (setup(py_params_dict) -> attributes_dict),
# not the class-method interface CreateVersatClass still depends on.
#
# Prerequisite: build the versat binary first, from the repository root:
#   make -j versat

import os
import subprocess

# The specification this example accelerator is generated from, and the
# top-level unit inside it to compile. Reuses the same accelerator as
# Section 2.3 of the Design Specification, so this wrapper's job is purely
# the py2hwsw integration, not a new accelerator design.
VERSAT_TOP = "SimpleExample"


def setup(py_params_dict):
    setup_dir = os.path.dirname(__file__)
    # setup_dir is examples/py2hwsw_example/iob_versat; the repository
    # root is three levels up.
    repo_root = os.path.abspath(os.path.join(setup_dir, "..", "..", ".."))

    versat_bin = os.path.join(repo_root, "versat")
    versat_spec = os.path.join(
        repo_root, "examples", "simple_example", "simple_example.versat"
    )
    hw_dir = os.path.join(setup_dir, "hardware", "src")
    sw_dir = os.path.join(setup_dir, "software")

    # Only actually invoke Versat during a real setup run, not during
    # py2hwsw's own dry-run/introspection targets (print_core_dict, etc).
    if py_params_dict.get("py2hwsw_target", "") == "setup":
        if not os.path.isfile(versat_bin):
            raise FileNotFoundError(
                f"{versat_bin} not found. Build it first, from the repository "
                "root: make -j versat"
            )

        os.makedirs(hw_dir, exist_ok=True)
        os.makedirs(sw_dir, exist_ok=True)
        subprocess.run(
            [
                versat_bin,
                versat_spec,
                "-t",
                VERSAT_TOP,
                "-o",
                hw_dir,
                "-O",
                sw_dir,
            ],
            check=True,
        )

    attributes_dict = {
        # iob_versat.v, the file Versat generates, is the module name;
        # generate_hw:False tells py2hwsw to use it instead of generating
        # a top Verilog file from this dictionary's ports/confs itself.
        "name": "iob_versat",
        "generate_hw": False,
        "description": (
            f"Standalone py2hwsw wrapper around the {VERSAT_TOP} accelerator "
            "Versat generates from examples/simple_example/simple_example.versat. "
            "See the Design Specification, Section 2.4."
        ),
        # A generic, low-cost board with pre-built py2hwsw support, so
        # `make fpga-build BOARD=iob_basys3` works without any board-specific
        # files of our own.
        "board_list": ["iob_basys3"],
        "confs": [
            {
                "name": "ADDR_W",
                "type": "P",
                "val": "7",
                "min": "1",
                "max": "32",
                "descr": (
                    "Address bus width. 7 is what Versat computed as "
                    "sufficient for this specific accelerator (the ADDR_W "
                    "stat line printed by the compilation command in "
                    "Section 2.3.1)."
                ),
            },
            {
                "name": "DATA_W",
                "type": "P",
                "val": "32",
                "min": "1",
                "max": "32",
                "descr": "Data bus width.",
            },
            {
                "name": "AXI_ADDR_W",
                "type": "P",
                "val": "32",
                "min": "1",
                "max": "32",
                "descr": (
                    "AXI address bus width. Unused by this accelerator, "
                    "since simple_example.versat does not use a databus "
                    "unit, but still a parameter of the generated module."
                ),
            },
            {
                "name": "AXI_DATA_W",
                "type": "P",
                "val": "32",
                "min": "1",
                "max": "32",
                "descr": "AXI data bus width. Unused by this accelerator.",
            },
            {
                "name": "AXI_LEN_W",
                "type": "P",
                "val": "8",
                "min": "1",
                "max": "8",
                "descr": "AXI burst length width. Unused by this accelerator.",
            },
            {
                "name": "AXI_ID_W",
                "type": "P",
                "val": "1",
                "min": "1",
                "max": "1",
                "descr": "AXI ID width. Unused by this accelerator.",
            },
            {
                "name": "WDATA_W",
                "type": "P",
                "val": "32",
                "min": "1",
                "max": "32",
                "descr": "Write data width.",
            },
            {
                "name": "MEM_ADDR_OFFSET",
                "type": "P",
                "val": "0",
                "min": "0",
                "max": "NA",
                "descr": "Offset of external memory address. Unused by this accelerator.",
            },
        ],
        # py2hwsw's built-in "iob" and "iob_clk" interfaces generate exactly
        # iob_versat.v's real port names for a subordinate ("_s") port:
        # iob_valid_i, iob_addr_i, iob_wdata_i, iob_wstrb_i, iob_rvalid_o,
        # iob_rdata_o, iob_ready_o, cke_i, clk_i, arst_i.
        "ports": [
            {
                "name": "iob_s",
                "descr": "IOb native slave interface, driven by the CPU or bus that owns this peripheral's address range.",
                "signals": {
                    "type": "iob",
                    "ADDR_W": "ADDR_W",
                    "DATA_W": "DATA_W",
                },
            },
            {
                "name": "clk_en_rst_s",
                "descr": "Clock, clock enable, and asynchronous reset",
                "signals": {
                    "type": "iob_clk",
                },
            },
        ],
        "wires": [],
        "subblocks": [],
        "snippets": [],
    }

    return attributes_dict
