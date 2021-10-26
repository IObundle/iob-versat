`timescale 1ns / 1ps
`include "xversat.vh"
`include "xdefs.vh"

`include "xconfdefs.vh"
`include "xmemdefs.vh"
`include "versat-io.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"

module xconf_reg # ( 
      parameter			    DATA_W = 32
) (
      input                      clk,
      input                      rst,
      
      //config interface
      output [`CONF_BITS-1:0]    conf_out,
`ifdef CONF_MEM_USE
      //Config mem interface
      input [`CONF_BITS-1:0]     conf_in,
      input                      conf_ld,
`endif

      //control interface
      input                      ctr_valid,
      input                      ctr_we,
      input [`CONF_REG_ADDR_W:0] ctr_addr,
      input [`IO_ADDR_W-1:0]     ctr_data_in
      );

   reg [`CONF_BITS-1:0]          conf_reg;
   wire                          conf_we;
   integer                       i;
      
   // assign output 
   assign conf_out=conf_reg;

   // determine write signal
   assign conf_we = ctr_valid & ctr_we;

   // update conf reg
   always @ (posedge rst, posedge clk) begin
      if(rst)
        conf_reg <= {`CONF_BITS{1'b0}};
  `ifdef CONF_MEM_USE
      else if (conf_ld)
        conf_reg <= conf_in;
  `endif
     else if (conf_we)
      if (ctr_addr == `CONF_CLEAR)
	      conf_reg <= {`CONF_BITS{1'b0}};
	else begin
	   
	   // configure MEMs
	  for (i=0; i<2*`nMEM; i=i+1) begin 
	     
	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_PER))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-`MEM_ADDR_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-`MEM_ADDR_W-`PERIOD_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_SEL))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W -: `N_W] <= ctr_data_in[`N_W-1:0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_START))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W -`N_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_SHIFT))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-2*`MEM_ADDR_W-2*`PERIOD_W -`N_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-3*`MEM_ADDR_W-2*`PERIOD_W -`N_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-4*`MEM_ADDR_W-2*`PERIOD_W -`N_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_RVRS))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-4*`MEM_ADDR_W-3*`PERIOD_W -`N_W -: 1] <= ctr_data_in[0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_EXT))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-4*`MEM_ADDR_W-3*`PERIOD_W -`N_W-1 -: 1] <= ctr_data_in[0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_IN_WR))
	       conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-4*`MEM_ADDR_W-3*`PERIOD_W -`N_W-2 -: 1] <= ctr_data_in[0];

	     if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER2))
               conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-4*`MEM_ADDR_W-3*`PERIOD_W -`N_W-3 -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

             if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_PER2))
               conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-5*`MEM_ADDR_W-3*`PERIOD_W -`N_W-3 -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

             if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_SHIFT2))
               conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-5*`MEM_ADDR_W-4*`PERIOD_W -`N_W-3 -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

             if(ctr_addr == (`CONF_MEM0A + i*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR2))
               conf_reg[`CONF_MEM0A_B-i*`MEMP_CONF_BITS-6*`MEM_ADDR_W-4*`PERIOD_W -`N_W-3 -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];  

	   end // for (i=0; i<2*`nMEM; i=i+1)

      // configure VIs
	  for (i=0; i<`nVI; i=i+1) begin

         if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_EXT_ADDR))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS -: `IO_ADDR_W] <= ctr_data_in[`IO_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_INT_ADDR))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_SIZE))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`MEM_ADDR_W -: `IO_SIZE_W] <= ctr_data_in[`IO_SIZE_W-1:0];

         if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_ITER_A))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-`MEM_ADDR_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_PER_A))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-2*`MEM_ADDR_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_DUTY_A))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-2*`MEM_ADDR_W-`PERIOD_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_SHIFT_A))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-2*`MEM_ADDR_W-2*`PERIOD_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_INCR_A))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-3*`MEM_ADDR_W-2*`PERIOD_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_ITER_B))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-4*`MEM_ADDR_W-2*`PERIOD_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_PER_B))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-5*`MEM_ADDR_W-2*`PERIOD_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_DUTY_B))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-5*`MEM_ADDR_W-3*`PERIOD_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_START_B))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-5*`MEM_ADDR_W-4*`PERIOD_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_SHIFT_B))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-6*`MEM_ADDR_W-4*`PERIOD_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_INCR_B))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-7*`MEM_ADDR_W-4*`PERIOD_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_DELAY_B))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-8*`MEM_ADDR_W-4*`PERIOD_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_RVRS_B))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-8*`MEM_ADDR_W-5*`PERIOD_W -: 1] <= ctr_data_in[0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_EXT_B))
	       conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-8*`MEM_ADDR_W-5*`PERIOD_W-1 -: 1] <= ctr_data_in[0];

	     if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_ITER2_B))
           conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-8*`MEM_ADDR_W-5*`PERIOD_W-2 -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

         if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_PER2_B))
           conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-9*`MEM_ADDR_W-5*`PERIOD_W-2 -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

         if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_SHIFT2_B))
           conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-9*`MEM_ADDR_W-6*`PERIOD_W-2 -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

         if(ctr_addr == (`CONF_VI0 + i*`VI_CONF_OFFSET + `VI_CONF_INCR2_B))
           conf_reg[`CONF_VI0_B-i*`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-10*`MEM_ADDR_W-6*`PERIOD_W-2 -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	   end // for (i=0; i<`nVI; i=i+1)

      // configure VOs
	  for (i=0; i<`nVO; i=i+1) begin

         if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_EXT_ADDR))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS -: `IO_ADDR_W] <= ctr_data_in[`IO_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_INT_ADDR))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_SIZE))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`MEM_ADDR_W -: `IO_SIZE_W] <= ctr_data_in[`IO_SIZE_W-1:0];

         if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_ITER_A))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-`MEM_ADDR_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_PER_A))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-2*`MEM_ADDR_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_DUTY_A))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-2*`MEM_ADDR_W-`PERIOD_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_SHIFT_A))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-2*`MEM_ADDR_W-2*`PERIOD_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_INCR_A))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-3*`MEM_ADDR_W-2*`PERIOD_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_ITER_B))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-4*`MEM_ADDR_W-2*`PERIOD_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_PER_B))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-5*`MEM_ADDR_W-2*`PERIOD_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_DUTY_B))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-5*`MEM_ADDR_W-3*`PERIOD_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_SEL_B))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-5*`MEM_ADDR_W-4*`PERIOD_W -: `N_W] <= ctr_data_in[`N_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_START_B))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-5*`MEM_ADDR_W-4*`PERIOD_W-`N_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_SHIFT_B))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-6*`MEM_ADDR_W-4*`PERIOD_W-`N_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_INCR_B))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-7*`MEM_ADDR_W-4*`PERIOD_W-`N_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_DELAY_B))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-8*`MEM_ADDR_W-4*`PERIOD_W-`N_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_RVRS_B))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-8*`MEM_ADDR_W-5*`PERIOD_W-`N_W -: 1] <= ctr_data_in[0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_EXT_B))
	       conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-8*`MEM_ADDR_W-5*`PERIOD_W-`N_W-1 -: 1] <= ctr_data_in[0];

	     if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_ITER2_B))
           conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-8*`MEM_ADDR_W-5*`PERIOD_W-`N_W-2 -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

         if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_PER2_B))
           conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-9*`MEM_ADDR_W-5*`PERIOD_W-`N_W-2 -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

         if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_SHIFT2_B))
           conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-9*`MEM_ADDR_W-6*`PERIOD_W-`N_W-2 -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

         if(ctr_addr == (`CONF_VO0 + i*`VO_CONF_OFFSET + `VO_CONF_INCR2_B))
           conf_reg[`CONF_VO0_B-i*`VO_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-10*`MEM_ADDR_W-6*`PERIOD_W-`N_W-2 -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	   end // for (i=0; i<`nVO; i=i+1)

	   // configure ALUs
	   for (i=0; i<`nALU; i=i+1) begin 
	      if(ctr_addr == (`CONF_ALU0 + i*`ALU_CONF_OFFSET + `ALU_CONF_SELA))
		conf_reg[`CONF_ALU0_B - i*`ALU_CONF_BITS -: `N_W] <= ctr_data_in[`N_W-1:0];

	      if(ctr_addr == (`CONF_ALU0 + i*`ALU_CONF_OFFSET + `ALU_CONF_SELB))
		conf_reg[`CONF_ALU0_B - i*`ALU_CONF_BITS - `N_W -: `N_W] <= ctr_data_in[`N_W-1:0];

	      if(ctr_addr == (`CONF_ALU0 + i*`ALU_CONF_OFFSET + `ALU_CONF_FNS))
		conf_reg[`CONF_ALU0_B - i*`ALU_CONF_BITS - 2*`N_W -: `ALU_FNS_W] <= ctr_data_in[`ALU_FNS_W-1:0];
	   end
	   
	   
	   // configure ALULITEs
	   for (i=0; i<`nALULITE; i=i+1) begin 
	      if(ctr_addr == (`CONF_ALULITE0 + i*`ALULITE_CONF_OFFSET + `ALULITE_CONF_SELA))
		conf_reg[`CONF_ALULITE0_B - i*`ALULITE_CONF_BITS -: `N_W] <= ctr_data_in[`N_W-1:0];

	      if(ctr_addr == (`CONF_ALULITE0 + i*`ALULITE_CONF_OFFSET + `ALULITE_CONF_SELB))
		conf_reg[`CONF_ALULITE0_B - i*`ALULITE_CONF_BITS - `N_W -: `N_W] <= ctr_data_in[`N_W-1:0];

	      if(ctr_addr == (`CONF_ALULITE0 + i*`ALULITE_CONF_OFFSET + `ALULITE_CONF_FNS))
		conf_reg[`CONF_ALULITE0_B - i*`ALULITE_CONF_BITS - 2*`N_W -: `ALULITE_FNS_W] <= ctr_data_in[`ALULITE_FNS_W-1:0];
	   end

	   // configure MULs
	   for (i=0; i<`nMUL; i=i+1) begin 
	      if(ctr_addr == (`CONF_MUL0 + i*`MUL_CONF_OFFSET + `MUL_CONF_SELA))
		conf_reg[`CONF_MUL0_B - i*`MUL_CONF_BITS -: `N_W] <= ctr_data_in[`N_W-1:0];

	      if(ctr_addr == (`CONF_MUL0 + i*`MUL_CONF_OFFSET + `MUL_CONF_SELB))
		conf_reg[`CONF_MUL0_B - i*`MUL_CONF_BITS - `N_W -: `N_W] <= ctr_data_in[`N_W-1:0];

	      if(ctr_addr == (`CONF_MUL0 + i*`MUL_CONF_OFFSET + `MUL_CONF_FNS))
		conf_reg[`CONF_MUL0_B - i*`MUL_CONF_BITS - 2*`N_W -: `MUL_FNS_W] <= ctr_data_in[`MUL_FNS_W-1:0];
	   end	   

	   // configure MULADDs
	   for (i=0; i<`nMULADD; i=i+1) begin 
	      if(ctr_addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_SELA))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS -: `N_W] <= ctr_data_in[`N_W-1:0];

	      if(ctr_addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_SELB))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS - `N_W -: `N_W] <= ctr_data_in[`N_W-1:0];

	      if(ctr_addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_FNS))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS - 2*`N_W -: `MULADD_FNS_W] <= ctr_data_in[`MULADD_FNS_W-1:0];

	      if(ctr_addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_ITER))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS - 2*`N_W - `MULADD_FNS_W -: `MEM_ADDR_W] <= ctr_data_in[`MEM_ADDR_W-1:0];

	      if(ctr_addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_PER))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS - 2*`N_W - `MULADD_FNS_W - `MEM_ADDR_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	      if(ctr_addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_DELAY))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS - 2*`N_W - `MULADD_FNS_W - `MEM_ADDR_W - `PERIOD_W -: `PERIOD_W] <= ctr_data_in[`PERIOD_W-1:0];

	      if(ctr_addr == (`CONF_MULADD0 + i*`MULADD_CONF_OFFSET + `MULADD_CONF_SHIFT))
		conf_reg[`CONF_MULADD0_B - i*`MULADD_CONF_BITS - 2*`N_W - `MULADD_FNS_W - `MEM_ADDR_W - 2*`PERIOD_W -: `SHIFT_W] <= ctr_data_in[`SHIFT_W-1:0];
	   end

	   // configure BSs
	   for (i=0; i<`nBS; i=i+1) begin 
	      if(ctr_addr == (`CONF_BS0 + i*`BS_CONF_OFFSET + `BS_CONF_SELD))
		conf_reg[`CONF_BS0_B - i*`BS_CONF_BITS -: `N_W] <= ctr_data_in[`N_W-1:0];

	      if(ctr_addr == (`CONF_BS0 + i*`BS_CONF_OFFSET + `BS_CONF_SELS))
		conf_reg[`CONF_BS0_B - i*`BS_CONF_BITS - `N_W -: `N_W] <= ctr_data_in[`N_W-1:0];

	      if(ctr_addr == (`CONF_BS0 + i*`BS_CONF_OFFSET + `BS_CONF_FNS))
		conf_reg[`CONF_BS0_B - i*`BS_CONF_BITS - 2*`N_W -: `BS_FNS_W] <= ctr_data_in[`BS_FNS_W-1:0];
	   end

	end // else: !if(addr == `CONF_CLEAR)
	
     end // always @ (posedge rst, posedge clk)
   

endmodule
