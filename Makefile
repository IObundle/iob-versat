SIM_DIR:=hardware/simulation/icarus
FPGA_DIR:=hardware/fpga/intel/cyclone_v_gt
PC_DIR:=software/pc/testbench

sim:
	make -C $(SIM_DIR)

fpga:
	make -C $(FPGA_DIR)

pc:
	make -C $(PC_DIR)

clean:
	-make -C $(SIM_DIR) clean
	-make -C $(FPGA_DIR) clean
	-make -C $(PC_DIR) clean


.PHONY: sim fpga pc clean
