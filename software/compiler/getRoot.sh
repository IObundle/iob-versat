printf "\`timescale 1ns / 1ps\nmodule Test(); endmodule" > /tmp/versat_test_verilator.v
verilator --trace --cc --Mdir $1/obj_dir /tmp/versat_test_verilator.v # Generate the makefile from which we extract VERILATOR_ROOT and the verilator .o and .cpp files needed
printf 'include Vversat_test_verilator.mk\nmyCode:\n\t@echo $(VERILATOR_ROOT)' > /tmp/versat_makefile1.mk
make --no-print-directory -I$1/obj_dir -f /tmp/versat_makefile1.mk myCode