`timescale 1ns / 1ps
`include "xversat.vh"
`include "xconfdefs.vh"

module xconf_tb;

	parameter            		clk_per = 20;
        integer				i;

        //inputs
	reg 				clk;
	reg 				rst;

        //config interface
	wire [`CONF_BITS-1:0]		conf_out;
  
        //control interface
	reg 				ctr_valid;
	reg 				ctr_we;
	reg [`CONF_REG_ADDR_W:0]	ctr_addr;
	reg [`MEM_ADDR_W-1:0]  		ctr_data_in;

	xconf uut (

  		//inputs
          	.clk(clk),
                .rst(rst),

		//config interface
                .conf_out(conf_out),

		//control interface
		.ctr_valid(ctr_valid),
		.ctr_we(ctr_we),
		.ctr_addr(ctr_addr),
		.ctr_data_in(ctr_data_in)
       );

`ifdef CONF_MEM_USE
       reg [`CONF_BITS-1:0]		conf_mem_last_data;
`endif

      initial begin
	 
`ifdef DEBUG
         $dumpfile("xconf.vcd");
         $dumpvars();
`endif

         //initialize Inputs
         clk = 0;
         rst = 1;
	 ctr_valid = 0;
	 ctr_we = 0;
         ctr_data_in = 0;
         ctr_addr = 0;

         //global reset (100 ns)
         #(clk_per*5) rst = 0;
         ctr_addr = `CONF_MEM0A;
         ctr_valid = 1;
         ctr_we = 1;
         ctr_data_in = {`MEM_ADDR_W{1'b1}};

         //test mem
         if(`nMEM > 0) begin
           $display("\nSetting MEM config bits to one");
           repeat(2*`nMEM)
             for(i = 0; i < `MEMP_CONF_OFFSET; i++) 
               #clk_per ctr_addr++;
           if(conf_out[`CONF_MEM0A_B -: 2*`nMEM*`MEMP_CONF_BITS] == {2*`nMEM*`MEMP_CONF_BITS{1'b1}})
	     $display("OK: All mem config bits set to one"); 
           else
	     $display("ERROR: Not all MEM config bits set to one");
         end

         //test ALUs
         if(`nALU > 0) begin
           $display("\nSetting ALU config bits to one");
           repeat(`nALU)
             for(i = 0; i < `ALU_CONF_OFFSET; i++)
               #clk_per ctr_addr++;
           if(conf_out[`CONF_ALU0_B -: `nALU*`ALU_CONF_BITS] == {`nALU*`ALU_CONF_BITS{1'b1}})
	     $display("OK: All ALU config bits set to one"); 
           else
	     $display("ERROR: Not all ALU config bits set to one");
         end

         //test ALULite
         if(`nALULITE > 0) begin
           $display("\nSetting ALULITE config bits to one");
           repeat(`nALULITE)
             for(i = 0; i < `ALULITE_CONF_OFFSET; i++)
               #clk_per ctr_addr++;
           if(conf_out[`CONF_ALULITE0_B -: `nALULITE*`ALULITE_CONF_BITS] == {`nALULITE*`ALULITE_CONF_BITS{1'b1}})
	     $display("OK: All ALULITE config bits set to one"); 
           else
	     $display("ERROR: Not all ALULITE config bits set to one");
         end
 
         //test MUL
         if(`nMUL > 0) begin
           $display("\nSetting MUL config bits to one");
           repeat(`nMUL)
             for(i = 0; i < `MUL_CONF_OFFSET; i++)
               #clk_per ctr_addr++;
           if(conf_out[`CONF_MUL0_B -: `nMUL*`MUL_CONF_BITS] == {`nMUL*`MUL_CONF_BITS{1'b1}})
	     $display("OK: All MUL config bits set to one"); 
           else
	     $display("ERROR: Not all MUL config bits set to one");
         end

         //test MULADD
         if(`nMULADD > 0) begin
           $display("\nSetting MULADD config bits to one");
           repeat(`nMULADD)
             for(i = 0; i < `MULADD_CONF_OFFSET; i++)
               #clk_per ctr_addr++;
           if(conf_out[`CONF_MULADD0_B -: `nMULADD*`MULADD_CONF_BITS] == {`nMULADD*`MULADD_CONF_BITS{1'b1}})
	     $display("OK: All MULADD config bits set to one"); 
           else
	     $display("ERROR: Not all MULADD config bits set to one");
         end

         //test BS
         if(`nBS > 0) begin
           $display("\nSetting BS config bits to one");
           repeat(`nBS)
             for(i = 0; i < `BS_CONF_OFFSET; i++)
               #clk_per ctr_addr++;
           if(conf_out[`CONF_BS0_B -: `nBS*`BS_CONF_BITS] == {`nBS*`BS_CONF_BITS{1'b1}})
	     $display("OK: All BS config bits set to one"); 
           else
	     $display("ERROR: Not all BS config bits set to one");
         end

`ifdef CONF_MEM_USE
	 //save the last config
	 conf_mem_last_data = conf_out;
         ctr_addr = `CONF_MEM;
         #clk_per;
`endif

         //trying to access out of range config position
         $display("\nTrying to access out of range config position");
         ctr_addr = `CONF_BS0  + `nBS*`BS_CONF_OFFSET;
	 #clk_per;

         //clear configs
         $display("\nClearing all configs");
         ctr_addr = `CONF_CLEAR;
	 #clk_per;
         if(conf_out == {`CONF_BITS{1'b0}})
	   $display("OK: All configs cleared\n"); 
         else
	   $display("ERROR: Not all configs cleared\n");

`ifdef CONF_MEM_USE
	 //load conf from xconf_mem to xconf_reg
	 $display("Loading last config saved in xconf_mem to xconf_reg");
	 ctr_we = 0;
         ctr_addr = `CONF_MEM;
         #(clk_per*2);
         if(conf_out == conf_mem_last_data)
	   $display("OK: Last config saved in mem loaded to reg\n");
         else
	   $display("ERROR: Last config saved in mem NOT loaded to reg\n");
`endif

         //end simulation
	 ctr_valid = 0;
	 ctr_we = 0;
         $finish;
      end
 

      always
         #(clk_per/2)  clk =  ~ clk;

endmodule
