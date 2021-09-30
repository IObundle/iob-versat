SIM_DIR:=hardware/simulation/icarus
FPGA_DIR:=hardware/fpga/intel/cyclone_v_gt
SW_DIR:=software

sim:
	make -C $(SIM_DIR)

fpga:
	make -C $(FPGA_DIR)

pc-sim:
	make -C $(SW_DIR)

clean:
	-make -C $(SIM_DIR) clean
	-make -C $(FPGA_DIR) clean
	-make -C $(SW_DIR) clean


.PHONY: sim fpga pc clean
