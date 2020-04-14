/*

 Data bus structure

 {0, 1, MEM0A, MEM0B, ..., ALU0, ..., ALULITE0, ..., MUL0, ..., MULADD0, ..., BS0, ...}


 Config bus structure

 {MEM0A, MEM0B, ..., ALU0, ..., ALULITE0, ..., MUL0, ..., MULADD0, ..., BS0, ...}

 */

`timescale 1ns / 1ps
`include "xversat.vh"

`include "xconfdefs.vh"
`include "xmemdefs.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"

module xdata_eng (
                  input                        clk,
                  input                        rst,

                  // control interface
                  input                        ctr_valid,
                  input                        ctr_we,
                  input [`nMEM_W+`DADDR_W:0]   ctr_addr,
                  input [`DATA_W-1:0]          ctr_data_in,
                  output reg [`DATA_W-1:0]     ctr_data_out,

                  // data interface
                  input                        data_valid,
                  input                        data_we,
                  input [`nMEM_W+`DADDR_W-1:0] data_addr,
                  input [`DATA_W-1:0]          data_data_in,
                  output reg [`DATA_W-1:0]     data_data_out,

                  //flow interface
                  input [`DATABUS_W-1:0]       flow_in, 
                  output [`DATABUS_W-1:0]      flow_out,          

                  // configuration bus
                  input [`CONF_BITS-1:0]       config_bus

                  );

   //WIDE ENGINE DATA BUS
   wire [2*`DATABUS_W-1:0]                     data_bus;
   
   //assign special data bus entries: constants 0 and 1
   assign data_bus[`DATA_S0_B -: `DATA_W] = `DATA_W'd0; //zero constant
   assign data_bus[`DATA_S1_B -: `DATA_W] = `DATA_W'd1; //one constant

   //flow interface
   assign data_bus[2*`DATABUS_W-1:`DATABUS_W] = flow_in;
   assign flow_out = data_bus[`DATABUS_W-1:0] ;


   //
   // CONTROL INTERFACE ADDRESS DECODER
   //

   //select control/status register or data memory 
   reg                                         control_valid;
   reg [`nMEM-1:0]                             mem_valid;
   
   always @ * begin
      integer j;
      control_valid = 1'b0;
      mem_valid = `nMEM'b0;
      if (ctr_addr[`nMEM_W+`DADDR_W])
        control_valid = ctr_valid;
      else
        for(j=0; j<`nMEM; j=j+1)
	  if ( j[`nMEM_W-1:0] == ctr_addr[`nMEM_W+`DADDR_W -: `nMEM_W] )
	    mem_valid[j] = ctr_valid;
   end

   //register selected data memory output
   reg [`DATA_W-1: 0] ctr_data_reg;
   always @ (posedge clk) begin
      integer j;
      for (j=0; j < `nMEM; j= j+1)
	if (mem_valid[j])
	  ctr_data_reg = data_bus[`DATA_MEM0A_B - (2*j+1)*`DATA_W  -: `DATA_W];
   end

   
   //read
   wire [2*`nMEM-1:0] mem_done;
   always @ * begin
      if(control_valid)
	ctr_data_out = {{`DATA_W-1{1'b0}}, &mem_done};
      else 
	ctr_data_out = ctr_data_reg;
   end


  // write
   always @ (posedge rst, posedge clk)
     if(rst) 
       ctr_reg <= 1'b0;
     else if(ctr_we)
       if(control_valid)
         ctr_reg <= ctr_data_in[0];

   wire run = control_valid & ctr_we &  ctr_data_in[0];
   wire run_reg = ctr_reg;
   
   
   //
   // DATA INTERFACE ADDRESS DECODER
   //

   // address register
   reg [`nMEM_W-1:0]                           data_addr_reg;
   always @ (posedge rst, posedge clk) begin
      if (rst) begin
	 data_addr_reg <= `nMEM_W'd0;
      end else begin
	 data_addr_reg <= data_addr[`nMEM_W + `DADDR_W -1 -: `nMEM_W];
      end
   end
   
   // select memory
   reg [`nMEM-1:0]                             data_mem_valid;
   always @ * begin
      integer j;
      data_mem_valid = {`nMEM{1'b0}};
      for (j = 0; j < `nMEM; j = j+1)
	if(data_addr[`nMEM_W + `DADDR_W -1 -: `nMEM_W] == j[`nMEM_W-1:0])
	  data_mem_valid[j] = data_valid;
   end

   //read: select output data
   always @ * begin
      integer j;
      data_data_out = `DATA_W'd0;
      for (j = 0; j < `nMEM; j = j+1)
	if(data_addr_reg == j[`nMEM_W-1:0])
	  data_data_out = data_bus[`DATA_MEM0A_B - 2*j*`DATA_W  -: `DATA_W];
   end

 
   // 
   // CONFIGURATION SHADOW REGISTER
   //
   reg [`CONF_BITS-1:0] config_reg_shadow;
   always @ (posedge rst, posedge clk) begin
      if(rst) begin
	 config_reg_shadow <= {`CONF_BITS{1'b0}};
   end
      else if(run) begin
	 config_reg_shadow <= config_bus;
      end
   end
   
   
   //
   // INSTANTIATE THE FUNCTIONAL UNITS
   //

   // generate iterator
   genvar                                      i;


   //
   // Instantiate the memories
   //
   //ICARUS does not support parameter arrays
   //parameter integer mem_size=4096;

   parameter integer MEM_ADDR_W[0 : `nMEM-1] =  `MEM_ADDR_W_DEF;
   parameter reg [`MEM_NAME_NCHARS*8-1:0] MEM_INIT_FILE[0:`nMEM-1] = `MEM_INIT_FILE_DEF;
   
   generate for (i=0; i < `nMEM; i=i+1) begin : mem_array
      //xmem  #(.mem_size(mem_size))  //for icarus
      xmem  #(
              .MEM_INIT_FILE(MEM_INIT_FILE[i]),
              .ADDR_W(MEM_ADDR_W[i])
              )
      mem (
	   .clk(clk),
	   .rst(rst),
	   .initA(run_reg),
	   .initB(run_reg),
	   .runA(run_reg),
	   .runB(run_reg),
	   .doneA(mem_done[2*i]),
	   .doneB(mem_done[2*i+1]),

	   // control interface
	   .ctr_mem_valid(mem_valid[i]),
	   .ctr_we(ctr_we),
	   .ctr_addr(ctr_addr[MEM_ADDR_W[i]-1:0]),
	   .ctr_data_in(ctr_data_in),
           
	   // data interface
	   .data_addr(data_addr[MEM_ADDR_W[i]-1:0]),
	   .data_we(data_we),
	   .data_mem_valid(data_mem_valid[i]),
	   .data_data_in(data_data_in),
           
	   // flow interface
	   .databus_in(data_bus),
	   .flow_out(data_bus[`DATA_MEM0A_B - 2*i*`DATA_W -: 2*`DATA_W]),
           
	   // configuration interface
	   .config_bits(config_reg_shadow[`CONF_MEM0A_B - 2*i*`MEMP_CONF_BITS -: 2*`MEMP_CONF_BITS])
	   );
   end
   endgenerate

   //
   // Instantiate the ALUs
   //
   generate
      for (i=0; i < `nALU; i=i+1) begin : add_array
	 xalu alu (
		   .clk(clk),
		   .rst(run_reg),

		   // flow interface
	           .databus_in(data_bus),
		   .flow_out(data_bus[`DATA_ALU0_B - i*`DATA_W -: `DATA_W]),

		   // configuration interface
		   .configdata(config_reg_shadow[`CONF_ALU0_B - i*`ALU_CONF_BITS -: `ALU_CONF_BITS])
		   );
      end
   endgenerate

   //
   // Instantiate the ALULITEs
   //
   generate
      for (i=0; i < `nALULITE; i=i+1) begin : add_LITE_array
	 xalulite aluLITE (
			   .clk(clk),
			   .rst(run_reg),

			   // flow interface
	                   .databus_in(data_bus),
			   .flow_out(data_bus[`DATA_ALULITE0_B - i*`DATA_W  -: `DATA_W]),

			   // configuration interface
			   .configdata(config_reg_shadow[`CONF_ALULITE0_B - i*`ALULITE_CONF_BITS -: `ALULITE_CONF_BITS])
			   );
      end
   endgenerate

   //
   // Instantiate the MULs
   //
   generate
      for (i=0; i < `nMUL; i=i+1) begin : mul_array
	 xmul mul (
		   .clk(clk),
		   .rst(run_reg),

		   // flow interface
	           .flow_in(data_bus),
		   .flow_out(data_bus[`DATA_MUL0_B - i*`DATA_W -: `DATA_W]),

		   // configuration interface
		   .configdata(config_reg_shadow[`CONF_MUL0_B - i*`MUL_CONF_BITS -: `MUL_CONF_BITS])
		   );
      end
   endgenerate

   //
   // Instantiate the MULADDs
   //
   generate
      for (i=0; i < `nMULADD; i=i+1) begin : muladd_array
	 xmuladd muladd (
		         .clk(clk),
		         .rst(run_reg),

			 // flow interface
	                 .flow_in(data_bus_prev),
			 .flow_out(data_bus[`DATA_MULADD0_B - i*`DATA_W -: `DATA_W]),
			 // configuration interface
			 .configdata(config_reg_shadow[`CONF_MULADD0_B - i*`MULADD_CONF_BITS -: `MULADD_CONF_BITS])
			 );
      end
   endgenerate

   //
   // Instantiate the BSs
   //
   generate
      for (i=0; i < `nBS; i=i+1) begin : bs_array
	 xbs bs (
		 .clk(clk),
		 .rst(run_reg),

		 // flow interface
	         .flow_in(data_bus_prev),
		 .flow_out(data_bus[`DATA_BS0_B - i*`DATA_W -: `DATA_W]),

		 // configuration interface
		 .configdata(config_reg_shadow[`CONF_BS0_B - i*`BS_CONF_BITS -: `BS_CONF_BITS])
		 );
      end
   endgenerate

endmodule
