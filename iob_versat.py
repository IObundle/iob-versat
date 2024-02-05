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

def RunVersat(pc_emul,versat_spec,versat_top,versat_extra,build_dir):
    versat_dir = os.path.dirname(__file__)

    versat_args = ["versat",os.path.realpath(versat_spec),
                            "-s",
                            "-b=32",
                            "-T",versat_top,
                            "-O",os.path.realpath(versat_dir + "/hardware/src/units"), # Location of versat units
                            #"-S",versat_dir + "/submodules/FPU/hardware/src/",
                            "-I",os.path.realpath(versat_dir + "/hardware/include/"),
                            "-I",os.path.realpath(versat_dir + "/hardware/src/"),
                            "-I",os.path.realpath(versat_dir + "/submodules/FPU/hardware/src/"),
                            "-I",os.path.realpath(versat_dir + "/submodules/FPU/hardware/include/"),
                            "-I",os.path.realpath(versat_dir + "/submodules/FPU/submodules/DIV/hardware/src/"),
                            "-I",os.path.realpath(build_dir  + "/hardware/src/"), # TODO: If this works then all the other "versat_dir + ..." could be removed
                            "-H",os.path.realpath(build_dir + "/software"), # Output software files
                            "-o",os.path.realpath(build_dir + "/hardware/src") # Output hardware files
                            ]

    if(versat_extra):
        versat_args = versat_args + ["-O",versat_extra]

    if(pc_emul):
        versat_args = versat_args + ["-x64"]

    print("\n",*versat_args,"\n",file=sys.stderr)
    result = sp.run(versat_args,capture_output=True)

    print(result,file=sys.stderr)
    
    returnCode = result.returncode
    output = codecs.getdecoder("unicode_escape")(result.stdout)[0]

    if(returnCode != 0):
        print("Failed to generate accelerator\n",file=sys.stderr)
        errorOutput = codecs.getdecoder("unicode_escape")(result.stderr)[0]
        print(output,file=sys.stderr)
        print(errorOutput,file=sys.stderr)

        exit(returnCode)

    lines = output.split('\n')

    return lines

def SaveSetupInfo(filepath,lines):
    try:
        with open(filepath,"w") as file:
           file.write("\n".join(lines))
    except:
        print(f"Failed to open versat setup file: {filepath}",file=sys.stderr)
        print("This might cause versat to run multiple times even if not needed",file=sys.stderr)

def CreateVersatClass(pc_emul,versat_spec,versat_top,versat_extra,build_dir):
    versat_dir = os.path.dirname(__file__)
    #build_dir = iob_module.build_dir

    versatSetupFilepath = os.path.realpath(build_dir + "/software/versatSetup.txt")
    alreadyRunned = os.path.isfile(versatSetupFilepath)

    # TODO: This still runs Versat 2 times if an error occurs. Probably better to save the fact that a error occured and exit twice
    lines = []
    if(alreadyRunned):
        try:
            with open(versatSetupFilepath,"r") as file:
               lines = [x.strip() for x in file.readlines()]
        except:
            lines = RunVersat(pc_emul,versat_spec,versat_top,versat_extra,build_dir)
            SaveSetupInfo(versatSetupFilepath,lines)
    else:
        lines = RunVersat(pc_emul,versat_spec,versat_top,versat_extra,build_dir)
        SaveSetupInfo(versatSetupFilepath,lines)

    print("Lines:",lines,file=sys.stderr)

    # Info needed by class, ADDR_W, HAS_AXI, lines
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
        USE_EXTMEM=HAS_AXI

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
                        "val": "1",
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
                        "val": "1",
                        "min": "?",
                        "max": "?",
                        "descr": "description",
                    },
                    {"name":"AXI_ADDR_W","type": "P","val": "24","min": "1","max": "32","descr": "AXI address bus width"}
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
            shutil.rmtree(f"{build_dir}/software/common")
            shutil.rmtree(f"{build_dir}/software/compiler")
            shutil.rmtree(f"{build_dir}/software/templates")
            shutil.rmtree(f"{build_dir}/software/tools")

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
                            "type": "RW",
                            "n_bits": ADDR_W,
                            "rst_val": 0,
                            "addr": (2**ADDR_W)-8, # -8 because -4 allocates 17 bits
                            "log2n_items": 0,
                            "autoreg": True,
                            "descr": "Force iob_soc to allocate address space for versat",
                        },
                    ],
                }
            ]

        @classmethod
        def _setup_confs(cls):
            confs = [
                {'name':'ADDR_W', 'type':'P', 'val':str(ADDR_W), 'min':'1', 'max':'?', 'descr':'description here'},
                {'name':'DATA_W', 'type':'P', 'val':'32', 'min':'1', 'max':'?', 'descr':'description here'},
            ]

            if(HAS_AXI):
                confs.append({"name":"USE_EXTMEM","type": "M","val": True,"min": "0","max": "1","descr": "Versat AXI implies External memory"})
                confs.append({'name':'AXI_ID_W', 'type':'P', 'val':'1', 'min':'1', 'max':'1', 'descr':'description here'})
                confs.append({'name':'AXI_LEN_W', 'type':'P', 'val':'8', 'min':'1', 'max':'8', 'descr':'description here'})
                confs.append({"name":"AXI_ADDR_W","type": "P","val": "30","min": "1","max": "32","descr": "AXI address bus width"}) # TODO: Changed 24 to 30. Realistically should receive from top the actual size.

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
                        "descr": "Versat AXI interface to connect directly to SDRAM",
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
