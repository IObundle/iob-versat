`timescale 1ns / 1ps
`include "xversat.vh"

`include "xconfdefs.vh"
`include "xmemdefs.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"

module xconf_reg (
      input                         clk,
      input                         rst,
      
      //data interface
      output [`CONF_BITS-1:0]       conf_out,
`ifdef CONF_MEM_USE
      //Config mem interface
      input [`CONF_BITS-1:0]        conf_in,
      input                         conf_ld,
`endif

      //control interface
      input                         req,
      input                         rnw,
      input [`CONF_REG_ADDR_W:0]    addr,
      input [`MEM_ADDR_W-1:0]          data_in
      );

      reg [`CONF_BITS-1:0]          conf_reg;
      wire                          conf_we;
      integer                       i;
      
   // assign output 
   assign conf_out=conf_reg;

   // determine write signal
   assign conf_we=req & ~rnw;

   // update conf reg
   always @ (posedge rst, posedge clk) begin
      if(rst)
        conf_reg <= {`CONF_BITS{1'b0}};
  `ifdef CONF_MEM_USE
      else if (conf_ld)
        conf_reg <= conf_in;
  `endif
     else if (conf_we)
      if (addr == `CONF_CLEAR)
	      conf_reg <= {`CONF_BITS{1'b0}};
	else begin
	   
	   // configure MEMs
	  for (i=0; i<2*`nMEM; i=i+1) begin 
	     
	     if(addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS -: `MEM_ADDR_W] <= data_in[`MEM_ADDR_W-1:0];

	     if(addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_PER))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-`MEM_ADDR_W -: `PERIOD_W] <= data_in[`PERIOD_W-1:0];
	     if(addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-`MEM_ADDR_W-`PERIOD_W -: `PERIOD_W] <= data_in[`PERIOD_W-1:0];

	     if(addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_SEL))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W -: `N_W] <= data_in[`N_W-1:0];
	     if(addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_START))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W -`N_W -: `MEM_ADDR_W] <= data_in[`MEM_ADDR_W-1:0];

	     if(addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_SHIFT))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-2*`MEM_ADDR_W-2*`PERIOD_W -`N_W -: `MEM_ADDR_W] <= data_in[`MEM_ADDR_W-1:0];

	     if(addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-3*`MEM_ADDR_W-2*`PERIOD_W -`N_W -: `MEM_ADDR_W] <= data_in[`MEM_ADDR_W-1:0];

	     if(addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-4*`MEM_ADDR_W-2*`PERIOD_W -`N_W -: `PERIOD_W] <= data_in[`PERIOD_W-1:0];

	     if(addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_RVRS))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-4*`MEM_ADDR_W-3*`PERIOD_W -`N_W -: 1] <= data_in[0];

	     if(addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_EXT))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-4*`MEM_ADDR_W-3*`PERIOD_W -`N_W-1 -: 1] <= data_in[0];

	   end // for (i=0; i<2*`nMEM; i=i+1)
	
	   // configure ALUs
	   for (i=0; i<`nALU; i=i+1) begin 
	      if(addr == (`CONF_ALU0 + i*`ALU_CONF_OFFSET + `ALU_CONF_SELA))
		conf_reg[`CONF_ALU0_B - i*`ALU_CONF_BITS -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_ALU0 + i*`ALU_CONF_OFFSET + `ALU_CONF_SELB))
		conf_reg[`CONF_ALU0_B - i*`ALU_CONF_BITS - `N_W -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_ALU0 + i*`ALU_CONF_OFFSET + `ALU_CONF_FNS))
		conf_reg[`CONF_ALU0_B - i*`ALU_CONF_BITS - 2*`N_W -: `ALU_FNS_W] <= data_in[`ALU_FNS_W-1:0];
	   end
	   
	   
	   // configure ALULITEs
	   for (i=0; i<`nALULITE; i=i+1) begin 
	      if(addr == (`CONF_ALULITE0 + i*`ALULITE_CONF_OFFSET + `ALULITE_CONF_SELA))
		conf_reg[`CONF_ALULITE0_B - i*`ALULITE_CONF_BITS -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_ALULITE0 + i*`ALULITE_CONF_OFFSET + `ALULITE_CONF_SELB))
		conf_reg[`CONF_ALULITE0_B - i*`ALULITE_CONF_BITS - `N_W -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_ALULITE0 + i*`ALULITE_CONF_OFFSET + `ALULITE_CONF_FNS))
		conf_reg[`CONF_ALULITE0_B - i*`ALULITE_CONF_BITS - 2*`N_W -: `ALULITE_FNS_W] <= data_in[`ALULITE_FNS_W-1:0];
	   end

	   // configure MULs
	   for (i=0; i<`nMUL; i=i+1) begin 
	      if(addr == (`CONF_MUL0 + i*`MUL_CONF_OFFSET + `MUL_CONF_SELA))
		conf_reg[`CONF_MUL0_B - i*`MUL_CONF_BITS -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_MUL0 + i*`MUL_CONF_OFFSET + `MUL_CONF_SELB))
		conf_reg[`CONF_MUL0_B - i*`MUL_CONF_BITS - `N_W -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_MUL0 + i*`MUL_CONF_OFFSET + `MUL_CONF_FNS))
		conf_reg[`CONF_MUL0_B - i*`MUL_CONF_BITS - 2*`N_W -: `MUL_FNS_W] <= data_in[`MUL_FNS_W-1:0];
	   end	   

	   // configure MULADDs
	   for (i=0; i<`nMULADD; i=i+1) begin 
	      if(addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_SELA))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_SELB))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS - `N_W -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_SELO))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS - 2*`N_W -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_FNS))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS - 3*`N_W -: `MULADD_FNS_W] <= data_in[`MULADD_FNS_W-1:0];
	   end

	   // configure BSs
	   for (i=0; i<`nBS; i=i+1) begin 
	      if(addr == (`CONF_BS0 + i*`BS_CONF_OFFSET + `BS_CONF_SELD))
		conf_reg[`CONF_BS0_B - i*`BS_CONF_BITS -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_BS0 + i*`BS_CONF_OFFSET + `BS_CONF_SELS))
		conf_reg[`CONF_BS0_B - i*`BS_CONF_BITS - `N_W -: `N_W] <= data_in[`N_W-1:0];

	      if(addr == (`CONF_BS0 + i*`BS_CONF_OFFSET + `BS_CONF_FNS))
		conf_reg[`CONF_BS0_B - i*`BS_CONF_BITS - 2*`N_W -: `BS_FNS_W] <= data_in[`BS_FNS_W-1:0];
	   end

	end // else: !if(addr == `CONF_CLEAR)
	
     end // always @ (posedge rst, posedge clk)
   

endmodule
