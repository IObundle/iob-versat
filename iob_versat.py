#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess as sp
import codecs

from iob_module import iob_module
from re import match

# Submodules
from iob_utils import iob_utils
from iob_regfile_sp import iob_regfile_sp
from iob_fifo_sync import iob_fifo_sync
from iob_ram_2p import iob_ram_2p
from iob_ram_sp import iob_ram_sp
from iob_reg import iob_reg
from iob_reg_re import iob_reg_re
from iob_ram_sp_be import iob_ram_sp_be
from iob_fp_fpu import iob_fp_fpu # Will also import all the other fp files

def CreateVersatClass(pc_emul,versat_spec,versat_top,versat_extra):
    versat_dir = os.path.dirname(__file__)
    build_dir = iob_module.build_dir

    versat_args = ["versat",versat_spec,
                            "-s",
                            "-b=32",
                            "-T",versat_top,
                            "-O",versat_dir + "/hardware/src/units", # Location of versat units
                            #"-S",versat_dir + "/submodules/FPU/hardware/src/",
                            "-I",versat_dir + "/hardware/include/",
                            "-I",versat_dir + "/hardware/src/",
                            "-I",versat_dir + "/submodules/FPU/hardware/src/",
                            "-I",versat_dir + "/submodules/FPU/hardware/include/",
                            "-I",versat_dir + "/submodules/FPU/submodules/DIV/hardware/src/",
                            "-I",build_dir  + "/hardware/src/", # TODO: If this works then all the other "versat_dir + ..." could be removed
                            "-H",build_dir + "/software", # Output software files
                            "-o",build_dir + "/hardware/src" # Output hardware files
                            ]

    if(versat_extra):
        versat_args = versat_args + ["-O",versat_extra]

    if(pc_emul):
        versat_args = versat_args + ["-x64"]

    # Set True to add debugger
    if(False):
        versat_args = ["gdb","-iex","set auto-load safe-path /","--args"] + versat_args

    print(*versat_args,file=sys.stderr)
    result = sp.run(versat_args,capture_output=True)

    returnCode = result.returncode
    output = codecs.getdecoder("unicode_escape")(result.stdout)[0]

    if(returnCode != 0):
        print("Failed to generate accelerator\n",file=sys.stderr)
        print(output,file=sys.stderr)

        exit(returnCode)

    lines = output.split('\n')

    print(lines,file=sys.stderr)

    ADDR_W = 32
    HAS_AXI = False

    for line in lines:
        tokens = line.split()

        if(len(tokens) == 3 and tokens[1] == '-'):
            if(tokens[0] == "ADDR_W"):
                ADDR_W = int(tokens[2])
            if(tokens[0] == "HAS_AXI"):
                HAS_AXI = True

    class iob_versat(iob_module):
        name = "iob_versat"
        version = "V0.10"
        flows = "pc-emul sim emb fpga"
        setup_dir=os.path.dirname(__file__)

        @classmethod
        def _init_attributes(cls):
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
                        "val": "4",
                        "min": "?",
                        "max": "?",
                        "descr": "description",
                    },
                    {
                        "name": "AXI_LEN_W",
                        "type": "M",
                        "val": "8",
                        "min": "?",
                        "max": "?",
                        "descr": "description",
                    },
                    {
                        "name": "AXI_ID",
                        "type": "M",
                        "val": "4",
                        "min": "?",
                        "max": "?",
                        "descr": "description",
                    },
                ]

        @classmethod
        def _post_setup(cls):
            super()._post_setup()
            shutil.copytree(
                f"{versat_dir}/hardware/src/units", f"{build_dir}/hardware/src",dirs_exist_ok = True
            )
            shutil.copytree(
                f"{build_dir}/hardware/src/modules", f"{build_dir}/hardware/src",dirs_exist_ok = True
            )
            #shutil.rmtree(f"{build_dir}/software/common")
            #shutil.rmtree(f"{build_dir}/software/compiler")
            #shutil.rmtree(f"{build_dir}/software/templates")
            #shutil.rmtree(f"{build_dir}/software/tools")

        @classmethod
        def _create_submodules_list(cls):
            ''' Create submodules list with dependencies of this module
            '''

            submodules = [iob_fifo_sync,iob_ram_sp,iob_ram_2p]

            if(True): # HAS_AXI
                submodules += [
                    {"interface": "axi_m_port"},
                    {"interface": "axi_m_m_portmap"},
                    {"interface": "axi_m_write_port"},
                    {"interface": "axi_m_m_write_portmap"},
                    {"interface": "axi_m_read_port"},
                    {"interface": "axi_m_m_read_portmap"},
                    #iob_fp_fpu # Imports all the FPU related files
                ]

            super()._create_submodules_list(submodules)

        @classmethod
        def _setup_regs(cls):
            cls.autoaddr = False
            cls.regs += [
                {
                    "name": "versat",
                    "descr": "VERSAT software accessible registers.",
                    "regs": [
                        {
                            "name": "MAX_CONFIG",
                            "type": "W",
                            "n_bits": ADDR_W,
                            "rst_val": 0,
                            "addr": (2**ADDR_W)-8, # -8 because -4 allocates 17 bits
                            "log2n_items": 0,
                            "autoreg": False,
                            "descr": "Force iob_soc to allocate 16 bits of address for versat",
                        },
                        {
                            "name": "MAX_STATUS",
                            "type": "R",
                            "n_bits": ADDR_W,
                            "rst_val": 0,
                            "addr": (2**ADDR_W)-8, # -8 because -4 allocates 17 bits
                            "log2n_items": 0,
                            "autoreg": False,
                            "descr": "Force iob_soc to allocate 16 bits of address for versat",
                        },
                    ],
                }
            ]

        @classmethod
        def _setup_confs(cls):
            confs = [
                {'name':'ADDR_W', 'type':'P', 'val':str(ADDR_W), 'min':'1', 'max':'?', 'descr':'description here'},
                {'name':'DATA_W', 'type':'P', 'val':'32', 'min':'1', 'max':'?', 'descr':'description here'},
                {'name':'AXI_ID_W', 'type':'P', 'val':'1', 'min':'1', 'max':'?', 'descr':'description here'},
                {'name':'AXI_LEN_W', 'type':'P', 'val':'8', 'min':'1', 'max':'?', 'descr':'description here'},
            ]

            if(HAS_AXI):
                confs.append({"name": "USE_EXTMEM","type": "M","val": True,"min": "0","max": "1","descr": "Versat AXI implies External memory"})

            super()._setup_confs(confs)

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
            if(HAS_AXI):
                cls.ios += [
                    {
                        "name": "axi_m_port",
                        "descr": "AXI interface",
                        "ports": [],
                    },
            ]

            # TODO: Actual good output directly from versat and parsing.
            for line in lines:
                tokens = line.split()

                if(len(tokens) == 3 and tokens[1] == '-'):
                    if(tokens[0] == "DP"):
                        inter,bitSize0,dataSizeOut0,dataSizeIn0,bitSize1,dataSizeOut1,dataSizeIn1 = [int(x) for x in tokens[2].split(',')]

                        cls.ios += [{"name": f"ext_mem_{inter}", "descr": "External memory", "ports": [
                            {"name": f"ext_dp_addr_{inter}_port_0_o","type": "O","n_bits": str(bitSize0),"descr": ""},
                            {"name": f"ext_dp_out_{inter}_port_0_o","type": "O","n_bits": str(dataSizeOut0),"descr": ""},
                            {"name": f"ext_dp_in_{inter}_port_0_i","type": "I","n_bits": str(dataSizeIn0),"descr": ""},
                            {"name": f"ext_dp_enable_{inter}_port_0_o","type": "O","n_bits": "1","descr": ""},
                            {"name": f"ext_dp_write_{inter}_port_0_o","type": "O","n_bits": "1","descr": ""},

                            {"name": f"ext_dp_addr_{inter}_port_1_o","type": "O","n_bits": str(bitSize1),"descr": ""},
                            {"name": f"ext_dp_out_{inter}_port_1_o","type": "O","n_bits": str(dataSizeOut1),"descr": ""},
                            {"name": f"ext_dp_in_{inter}_port_1_i","type": "I","n_bits": str(dataSizeIn1),"descr": ""},
                            {"name": f"ext_dp_enable_{inter}_port_1_o","type": "O","n_bits": "1","descr": ""},
                            {"name": f"ext_dp_write_{inter}_port_1_o","type": "O","n_bits": "1","descr": ""},
                        ]}]
                    if(tokens[0] == "2P"):
                        inter,bitSizeOut,bitSizeIn,dataSizeOut,dataSizeIn = [int(x) for x in tokens[2].split(',')]
                        cls.ios += [{"name": f"ext_mem_{inter}", "descr": "External memory", "ports": [
                            {"name": f"ext_2p_addr_out_{inter}_o","type": "O","n_bits": str(bitSizeOut),"descr": ""},
                            {"name": f"ext_2p_addr_in_{inter}_o","type": "O","n_bits": str(bitSizeIn),"descr": ""},
                            {"name": f"ext_2p_write_{inter}_o","type": "O","n_bits": "1","descr": ""},
                            {"name": f"ext_2p_read_{inter}_o","type": "O","n_bits": "1","descr": ""},
                            {"name": f"ext_2p_data_in_{inter}_i","type": "I","n_bits": str(dataSizeIn),"descr": ""},
                            {"name": f"ext_2p_data_out_{inter}_o","type": "O","n_bits": str(dataSizeOut),"descr": ""},
                        ]}]

        @classmethod
        def _setup_block_groups(cls):
            cls.block_groups += []

    return iob_versat
