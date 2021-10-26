#!/bin/bash

quartus_sh --64bit -t compile.tcl

#quartus_asm top_level --64bit --read_settings_files=on --write_settings_files=off -c arria_gx
#quartus_pgm -m jtag -c 1 -o "p;arria_gx_time_limited.sof"
