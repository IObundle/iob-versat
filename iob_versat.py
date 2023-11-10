#!/usr/bin/env python3

import os
import sys
import subprocess as sp

from iob_module import iob_module

# Submodules
from iob_utils import iob_utils
from iob_regfile_sp import iob_regfile_sp
from iob_fifo_sync import iob_fifo_sync
from iob_ram_2p import iob_ram_2p
from iob_ram_sp import iob_ram_sp
from iob_reg import iob_reg
from iob_reg_re import iob_reg_re
from iob_ram_sp_be import iob_ram_sp_be

class iob_versat(iob_module):
    name = "iob_versat"
    version = "V0.10"
    flows = "sim emb"
    setup_dir=os.path.dirname(__file__)

    def __init__(
        self,
        name="",
        description="default description",
        parameters={},
    ):
        super().__init__(name,description,parameters)

        versat_spec = parameters["versat_spec"]
        versat_top = parameters["versat_top"]
        versat_extra = parameters["extra_units"]

        versat_dir = os.path.dirname(__file__)
        build_dir = self.build_dir

        versat_args = ["versat",versat_spec,
                                "-s",
                                "-T",versat_top,
                                "-O",versat_dir + "/hardware/src/units",
                                "-O",versat_extra,
                                "-H",build_dir + "/software",
                                "-o",build_dir + "/hardware/src"
                                ]

        print(*versat_args)
        result = sp.call(versat_args)

        if(result != 0):
            exit(result)

        #sp.call(["verilator"])
        #sp.call(["versat","versat_spec","-T","versat_top"])
        #exit(0)

    @classmethod
    def _init_attributes(cls):
        # Do not know how useful is this, it depends upon the accelerator generated by the user
        cls.AXI_CONFS = [
                {
                    "name": "AXI",
                    "type": "M",
                    "val": "NA",
                    "min": "NA",
                    "max": "NA",
                    "descr": "AXI interface",
                },
                {
                    "name": "AXI_ID_W",
                    "type": "M",
                    "val": "1",
                    "min": "?",
                    "max": "?",
                    "descr": "description",
                },
                {
                    "name": "AXI_LEN_W",
                    "type": "M",
                    "val": "4",
                    "min": "?",
                    "max": "?",
                    "descr": "description",
                },
                {
                    "name": "AXI_ID",
                    "type": "M",
                    "val": "0",
                    "min": "?",
                    "max": "?",
                    "descr": "description",
                },
            ]

    @classmethod
    def _create_submodules_list(cls):
        ''' Create submodules list with dependencies of this module
        '''
        super()._create_submodules_list([
            {"interface": "axi_m_m_portmap"},
            {"interface": "axi_m_write_port"},
            {"interface": "axi_m_m_write_portmap"},
            {"interface": "axi_m_read_port"},
            {"interface": "axi_m_m_read_portmap"},
        ])

    @classmethod
    def _setup_regs(cls):
        cls.autoaddr = False
        cls.regs += [
            {
                "name": "versat",
                "descr": "VERSAT software accessible registers.",
                "regs": [
                    {
                        "name": "CONFIG",
                        "type": "W",
                        "n_bits": 32,
                        "rst_val": 0,
                        "addr": 0,
                        "log2n_items": 0,
                        "autoreg": False,
                        "descr": "CONFIG",
                    },
                    {
                        "name": "STATUS",
                        "type": "R",
                        "n_bits": 32,
                        "rst_val": 0,
                        "addr": 0,
                        "log2n_items": 0,
                        "autoreg": False,
                        "descr": "STATUS",
                    },
                ],
            }
        ]

    @classmethod
    def _setup_confs(cls):
        super()._setup_confs([
            # Macros

            # Parameters
            {'name':'ADDR_W', 'type':'P', 'val':'32', 'min':'1', 'max':'?', 'descr':'description here'},
            {'name':'DATA_W', 'type':'P', 'val':'32', 'min':'1', 'max':'?', 'descr':'description here'},
        ])

    @classmethod
    def _setup_ios(cls):
        cls.ios += [
            {
                "name": "clk_en_rst_s_port",
                "descr": "Clock, clock enable and reset",
                "ports": [],
            },
            {"name": "iob_s_port", "descr": "CPU native interface", "ports": []},
        ]

    @classmethod
    def _setup_block_groups(cls):
        pass
        #cls.block_groups += []
