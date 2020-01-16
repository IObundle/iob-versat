/*

 Data bus structure

 {0, 1, MEM0A, MEM0B, ..., ALU0, ..., ALULITE0, ..., MUL0, ..., MULADD0, ..., BS0, ...}


 Config bus structure

 {MEM0A, MEM0B, ..., ALU0, ..., ALULITE0, ..., MUL0, ..., MULADD0, ..., BS0, ...}

 */

`timescale 1ns / 1ps
`include "xdefs.vh"
`include "xdata_engdefs.vh"
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
                 
                 // Controller interface
                 input                        ctr_req,
                 input                        run_req,
                 input                        ctr_rnw,
                 input      [`ENG_ADDR_W-1:0] ctr_addr,
                 input      [`DATA_W-1:0]     ctr_data_to_wr,
                 output reg [`DATA_W-1:0]     ctr_data_to_rd,
                 
                 // Configuration bus
                 input      [`CONF_BITS-1:0]  config_bus,
                 
                 // DATABUS interface
                 input                        databus_req,
                 input                        databus_rnw,
                 input [`nMEM_W+`DADDR_W-1:0] databus_addr,
                 input [`DATA_W-1:0]          databus_data_in,
                 output reg [`DATA_W-1:0]     databus_data_out,
                 
                 input  [`DATA_BITS-1:0]      data_bus_prev,       
                 output [`DATA_BITS-1:0]      data_bus            

                 );

   //run and ready signals
   wire                            ready;
   wire                            run;

   // run and data registers
   reg                             run_reg;
   reg  [`DATA_W-1: 0]             data_reg;

   // configuration shadow register
   reg  [`CONF_BITS-1:0] 		       config_reg_shadow;

   // internal control signals
   wire [2*`nMEM-1:0] 			       mem_done;

   //address decoder requests 
   reg                             ready_req;
   reg  [`nMEM-1:0]                mem_req;

   // databus signals
   reg  [`nMEM-1:0]                databus_mem_req;
   reg  [`nMEM_W-1:0]              databus_addr_reg;

   // generate iterators
   genvar                          i;

   // assign run and ready signals
   assign run     = run_req & ~ctr_rnw;
   assign ready   = &mem_done;

   // run bit register
   always @ (posedge rst, posedge clk)
      if(rst) 
	      run_reg <= 1'b0;
      else 
	      run_reg <= run;

   //assign special data bus entries: constants 0 and 1
   assign data_bus[`DATA_S0_B -: `DATA_W] = `DATA_W'd0; //zero constant
   assign data_bus[`DATA_S1_B -: `DATA_W] = `DATA_W'd1; //one constant

   // configuration shadow register
   always @ (posedge rst, posedge clk) begin
      if(rst) begin
	      config_reg_shadow <= {`CONF_BITS{1'b0}};
      end
      else if(run) begin
	      config_reg_shadow <= config_bus;
      end
   end

   // databus address register
   always @ (posedge rst, posedge clk) begin
      if (rst) begin
	      databus_addr_reg <= `nMEM_W'd0;
      end else begin
	      databus_addr_reg <= databus_addr[`nMEM_W + `DADDR_W -1 -: `nMEM_W];
      end
   end


   //
   // DATABUS address decoder
   //

   //compute requests
   always @ * begin
      integer j;
      databus_mem_req = {`nMEM{1'b0}};
      for (j = 0; j < `nMEM; j = j+1)
	      if(databus_addr[`nMEM_W + `DADDR_W -1 -: `nMEM_W] == j[`nMEM_W-1:0])
	        databus_mem_req[j] = databus_req;
   end
   //compute databus_data_out
   always @ * begin
      integer j;
      databus_data_out = `DATA_W'd0;
      for (j = 0; j < `nMEM; j = j+1)
	      if(databus_addr_reg == j[`nMEM_W-1:0])
	        databus_data_out = data_bus[`DATA_MEM0A_B - 2*j*`DATA_W  -: `DATA_W];
   end

   //
   // Controller address decoder
   //

   //compute requests
   always @ * begin
     integer j;
     ready_req = 1'b0;
     mem_req = `nMEM'b0;
     if (ctr_addr == `ENG_RDY_REG)
       ready_req = ctr_req;
     else
       for(j=0; j<`nMEM; j=j+1)
	        if ( j[`nMEM_W-1:0] == ctr_addr[`ENG_ADDR_W-1 -: `nMEM_W] )
	          mem_req[j] = ctr_req;
   end // block: addr_decoder

   //compute data to read
   always @ * begin
      ctr_data_to_rd = `DATA_W'd0;
      if(ready_req)
	      ctr_data_to_rd = {{`DATA_W-1{1'b0}},ready};
      else 
	      ctr_data_to_rd = data_reg;
   end // always @ *
   
   always @ (posedge clk) begin
      integer j;
      for (j=0; j < `nMEM; j= j+1)
	        if (mem_req[j])
	          data_reg = data_bus[`DATA_MEM0A_B - (2*j+1)*`DATA_W  -: `DATA_W];
   end
   
   // Instantiate the data memories

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

	   // Controller interface
	   .ctr_mem_req(mem_req[i]),
	   .ctr_rnw(ctr_rnw),
	   .ctr_addr(ctr_addr[MEM_ADDR_W[i]-1:0]),
	   .ctr_data_to_wr(ctr_data_to_wr),
           
	   // DATABUS interface
	   .databus_addr(databus_addr[MEM_ADDR_W[i]-1:0]),
	   .databus_rnw(databus_rnw),
	   .databus_mem_req(databus_mem_req[i]),
	   .databus_data_in(databus_data_in),
           
	   // Data IO
	   .data_bus_prev(data_bus_prev),
	   .data_bus(data_bus),
	   .outA(data_bus[`DATA_MEM0A_B - 2*i*`DATA_W -: `DATA_W]),
	   .outB(data_bus[`DATA_MEM0A_B - (2*i+1)*`DATA_W -: `DATA_W]),
           
	   // Configuration data
	   .config_bits(config_reg_shadow[`CONF_MEM0A_B - 2*i*`MEMP_CONF_BITS -: 2*`MEMP_CONF_BITS])
	   );
   end
   endgenerate

   // Instantiate the ALUs
   generate
      for (i=0; i < `nALU; i=i+1) begin : add_array
	 xalu alu (
		   .clk(clk),
		   .rst(run_reg),

		   // Data IO
	     .data_bus_prev(data_bus_prev),
		   .data_bus(data_bus),
		   .result(data_bus[`DATA_ALU0_B - i*`DATA_W -: `DATA_W]),

		   // Configuration data
		   .configdata(config_reg_shadow[`CONF_ALU0_B - i*`ALU_CONF_BITS -: `ALU_CONF_BITS])
		   );
      end
   endgenerate

   // Instantiate the ALULITEs
   generate
      for (i=0; i < `nALULITE; i=i+1) begin : add_LITE_array
	 xalulite aluLITE (
			   .clk(clk),
			   .rst(run_reg),

			   // Data IO
	       .data_bus_prev(data_bus_prev),
			   .data_bus(data_bus),
			   .result(data_bus[`DATA_ALULITE0_B - i*`DATA_W  -: `DATA_W]),

			   // Configuration data
			   .configdata(config_reg_shadow[`CONF_ALULITE0_B - i*`ALULITE_CONF_BITS -: `ALULITE_CONF_BITS])
			   );
      end
   endgenerate

   // Instantiate the MULs
   generate
      for (i=0; i < `nMUL; i=i+1) begin : mul_array
	 xmul mul (
		   .clk(clk),
		   .rst(run_reg),

		   // Data IO
	     .data_bus_prev(data_bus_prev),
		   .data_bus(data_bus),
		   .result(data_bus[`DATA_MUL0_B - i*`DATA_W -: `DATA_W]),

		   // Configuration data
		   .configdata(config_reg_shadow[`CONF_MUL0_B - i*`MUL_CONF_BITS -: `MUL_CONF_BITS])
		   );
      end
   endgenerate

   // Instantiate the MULADDs
   generate
      for (i=0; i < `nMULADD; i=i+1) begin : muladd_array
	 xmuladd muladd (
		            .clk(clk),
		            .rst(run_reg),

			    // Data IO
	        .data_bus_prev(data_bus_prev),
			    .data_bus(data_bus),
			    .result(data_bus[`DATA_MULADD0_B - i*`DATA_W -: `DATA_W]),

			    // Configuration data
			    .configdata(config_reg_shadow[`CONF_MULADD0_B - i*`MULADD_CONF_BITS -: `MULADD_CONF_BITS])
			    );
      end
   endgenerate

   // Instantiate the BSs
   generate
      for (i=0; i < `nBS; i=i+1) begin : bs_array
	 xbs bs (
		 .clk(clk),
		 .rst(run_reg),

		 // Data IO
	   .data_bus_prev(data_bus_prev),
		 .data_bus(data_bus),
		 .result(data_bus[`DATA_BS0_B - i*`DATA_W -: `DATA_W]),

		 // Configuration data
		 .configdata(config_reg_shadow[`CONF_BS0_B - i*`BS_CONF_BITS -: `BS_CONF_BITS])
		 );
      end
   endgenerate

endmodule
