all:
	@rm -f xversat.log
	vivado -nojournal -log xversat.log -mode batch -source xversat.tcl -tclargs
	@-grep -B4 -A10 'CLB LUTs' xversat.log
	@-grep -B1 -A9 ^Slack xversat.log && echo

clean:
	@rm -rf .Xil/ *.log 

.PHONY: all clean FORCE
