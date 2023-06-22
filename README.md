Versat is a Coarse Grained Reconfigurable Array (CGRA) IP core

## Cite this project

If you use this project please cite the following publication:

Lopes, J.D.; Véstias, M.P.; Duarte , R.P.; Neto, H.C.; de Sousa , J.T. 
Coarse-Grained Reconfigurable Computing with the Versat Architecture. 
Electronics 2021, 10, 669. https://doi.org/10.3390/electronics10060669 


## Simulate

#Edit simulator path in Makefile and do:

```
make sim
```

## Compile FPGA 

#Edit FPGA path in Makefile and do:

```
source path/to/vivado/settings64.sh
make fpga
```

## Setup Verilator
- Install the Verilator simulator following the instructions in the 
[official page](https://verilator.org/guide/latest/install.html).
- Set the `VERILATOR_INCLUDE` environment variable in `$HOME/.bashrc`:
```
export VERILATOR_INCLUDE=<path>/share/verilator/include
```
  - The `<path>` to Verilator can be obtained with the command `which verilator` 
  after installation. For example if `which verilator` returns 
  `/usr/local/bin/verilog`, therefore: `<path>`=`/usr/local`. Final command:
  `export VERILATOR_INCLUDE=/usr/local/share/verilator/include`

