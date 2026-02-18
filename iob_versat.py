#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess as sp
import codecs

from iob_module import iob_module

# This class is not intended to be used.
# It exists because some tools/scripts only expect to find a class 
# here. Any SoC system that wants to integrate Versat needs to call the
# CreateVersatClass to create the actual class.
class iob_versat(iob_module):
    name = "iob_versat"
    version = "V0.10"
    flows = "sim emb"
    setup_dir = os.path.dirname(__file__)

    @classmethod
    def _create_submodules_list(cls):
        ''' Create submodules list with dependencies of this module
        '''

        submodules = []
        if(True): # HAS_AXI
            submodules += [
                {"interface": "axi_m_port"},
                {"interface": "axi_m_m_portmap"},
                {"interface": "axi_m_write_port"},
                {"interface": "axi_m_m_write_portmap"},
                {"interface": "axi_m_read_port"},
                {"interface": "axi_m_m_read_portmap"},
            ]

        super()._create_submodules_list(submodules)

    @classmethod
    def _setup_confs(cls):
        super()._setup_confs(
            [
                # Macros
                # Parameters
                {
                    "name": "DATA_W",
                    "type": "P",
                    "val": "32",
                    "min": "NA",
                    "max": "NA",
                    "descr": "Data bus width",
                },
                {
                    "name": "ADDR_W",
                    "type": "P",
                    "val": "`IOB_VERSAT_SWREG_ADDR_W",
                    "min": "NA",
                    "max": "NA",
                    "descr": "Address bus width",
                },
                {
                    "name": "WDATA_W",
                    "type": "P",
                    "val": "1",
                    "min": "NA",
                    "max": "8",
                    "descr": "",
                },
                {
                    "name": "MEM_ADDR_OFFSET",
                    "type": "P",
                    "val": "0",
                    "min": "0",
                    "max": "NA",
                    "descr": "Offset of memory address",
                },
            ]
        )

    @classmethod
    def _setup_ios(cls):
        cls.ios += [
            {"name": "iob_s_port", "descr": "CPU native interface", "ports": []},
            {
                "name": "general",
                "descr": "General interface signals",
                "ports": [
                    {
                        "name": "clk_i",
                        "type": "I",
                        "n_bits": "1",
                        "descr": "System clock input",
                    },
                ],
            },
        ]

    @classmethod
    def _setup_regs(cls):
        cls.regs += [
            {
                "name": "timer",
                "descr": "TIMER software accessible registers.",
                "regs": [
                    {
                        "name": "RESET",
                        "type": "W",
                        "n_bits": 1,
                        "rst_val": 0,
                        "log2n_items": 0,
                        "autoreg": True,
                        "descr": "Timer soft reset",
                    },
                ],
            }
        ]

    @classmethod
    def _setup_block_groups(cls):
        cls.block_groups += []


def RunVersat(versat_spec,versat_top,versat_extra,build_dir,axi_data_w,debug_path,profile):
    versat_dir = os.path.dirname(__file__)

    versat_args = ["versat",os.path.realpath(versat_spec),
                    "--debug",
                    f"-b{axi_data_w}",
                    "-d", # DMA
                    "-t",versat_top,
                    "-o",os.path.realpath(build_dir + "/hardware/src"), # Output hardware files
                    "-O",os.path.realpath(build_dir + "/software") # Output software files
                    ]

    if(debug_path):
        versat_args = versat_args + ["-g",debug_path]

    if(versat_extra):
        versat_args = versat_args + ["-u",versat_extra]

    if(profile):
        versat_args.append("--profile")

    print(*versat_args,"\n",file=sys.stderr)
    result = None
    try:
        result = sp.run(versat_args,capture_output=True)
    except:
        return []

    returnCode = result.returncode
    output = codecs.getdecoder("unicode_escape")(result.stdout)[0]

    errorOutput = codecs.getdecoder("unicode_escape")(result.stderr)[0]
    print(output,file=sys.stderr)
    print(errorOutput,file=sys.stderr)

    if(returnCode != 0):
        print("Failed to generate accelerator\n",file=sys.stderr)

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

def CreateVersatClass(versat_spec,versat_top,versat_extra,build_dir,axi_data_w,debug_path=None,profile=None):
    versat_dir = os.path.dirname(__file__)

    versatSetupFilepath = os.path.realpath(build_dir + "/software/versatSetup.txt")
    alreadyRunned = os.path.isfile(versatSetupFilepath)

    # TODO: This still runs Versat 2 times if an error occurs. Probably better to save the fact that a error occured and exit twice
    lines = []
    if(alreadyRunned):
        try:
            with open(versatSetupFilepath,"r") as file:
               lines = [x.strip() for x in file.readlines()]
        except:
            lines = RunVersat(versat_spec,versat_top,versat_extra,build_dir,axi_data_w,debug_path,profile)
            SaveSetupInfo(versatSetupFilepath,lines)
    else:
        lines = RunVersat(versat_spec,versat_top,versat_extra,build_dir,axi_data_w,debug_path,profile)
        SaveSetupInfo(versatSetupFilepath,lines)

    # Info needed by class, ADDR_W, HAS_AXI, lines
    ADDR_W = 32
    AXI_DATA_W = axi_data_w
    HAS_AXI = False

    for line in lines:
        tokens = line.split(":")

        if(len(tokens) == 2):
            if(tokens[0] == "ADDR_W"):
                ADDR_W = int(tokens[1])
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
                        "min": "1",
                        "max": "1",
                        "descr": "description",
                    },
                    {
                        "name": "AXI_LEN_W",
                        "type": "M",
                        "val": "?",
                        "min": "1",
                        "max": "8",
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
                    {
                        "name": "AXI_DATA_W",
                        "type": "P",
                        "val": f"{AXI_DATA_W}",
                        "min": "0",
                        "max": "256",
                        "descr": "Versat AXI datapath size",
                    },
                    {"name":"AXI_ADDR_W","type": "P","val": "32","min": "1","max": "32","descr": "AXI address bus width"}
                ]

        @classmethod
        def _post_setup(cls):
            super()._post_setup()

            shutil.rmtree(f"{build_dir}/software/common")
            shutil.rmtree(f"{build_dir}/software/compiler")
            shutil.rmtree(f"{build_dir}/software/templates")
            shutil.rmtree(f"{build_dir}/software/tools")

        @classmethod
        def _create_submodules_list(cls):
            ''' Create submodules list with dependencies of this module
            '''

            submodules = []
            if(True): # HAS_AXI
                submodules += [
                    {"interface": "axi_m_port"},
                    {"interface": "axi_m_m_portmap"},
                    {"interface": "axi_m_write_port"},
                    {"interface": "axi_m_m_write_portmap"},
                    {"interface": "axi_m_read_port"},
                    {"interface": "axi_m_m_read_portmap"},
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
                {
                    "name": "MEM_ADDR_OFFSET",
                    "type": "P",
                    "val": "0",
                    "min": "0",
                    "max": "NA",
                    "descr": "Offset of memory address",
                },
            ]

            if(HAS_AXI):
                confs.append({"name":"USE_EXTMEM","type": "M","val": True,"min": "0","max": "1","descr": "Versat AXI implies External memory"})
                confs.append({'name':'AXI_ID_W', 'type':'P', 'val':'1', 'min':'1', 'max':'1', 'descr':'description here'})
                confs.append({'name':'AXI_LEN_W', 'type':'P', 'val':'8', 'min':'1', 'max':'8', 'descr':'description here'})
                confs.append({"name":"AXI_ADDR_W","type": "P","val": "32","min": "1","max": "32","descr": "AXI address bus width"}) # TODO: Changed 24 to 30. Realistically should receive from top the actual size.
                confs.append({"name":"AXI_DATA_W","type": "P","val": f"{AXI_DATA_W}","min": "1","max": "256","descr": "AXI data bus width"})

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
