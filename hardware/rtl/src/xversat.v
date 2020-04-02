`timescale 1ns / 1ps

// top defines
`include "xdefs.vh"
`include "xctrldefs.vh"
`include "xconfdefs.vh"
`include "xdata_engdefs.vh"

// FU defines
`include "xmemdefs.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"
`include "xdivdefs.vh"

module xversat
            (
	     //Control Bus
   	   input 			  	         ctr_req,
   	   input 			  	         run_req,
   	   input 			             ctr_rnw,
   	   input  [`ADDR_W-1:0] 	 ctr_addr,
       input  [`DATA_W-1:0]    ctr_data_to_wr,
   	   output [`DATA_W-1:0]    ctr_data_to_rd,

	     //databus slave interface / DATA i/f
	     input 				           databus_req,
	     input 				           databus_rnw,
	     input [`ADDR_W-1:0]     databus_addr,
	     input [`DATA_W-1:0]     databus_data_in,
	     output[`DATA_W-1:0]     databus_data_out,
      
       //signals used to connect 2 Versats      
       input  [`DATA_BITS-1:0] databus_data_prev,       
       output [`DATA_BITS-1:0] databus_data_next,       

	     input 				       clk,
       input 				       rst       	     
       );




   //data engine
   wire                     eng_req;
   wire [`DATA_W-1:0]       eng_data_to_rd;

   //configuration module
   wire 			  conf_req;

`ifdef DEBUG
   //printer
   wire 			  prt_req;
`endif



   //
   // databus ADDRESS DECODER
   //

 `ifdef PROG_MEM_USE
   //prog memory
   wire 			        databus_prog_req;
   wire [`DATA_W-1:0] databus_data_from_prog;
 `endif
   //data engine
   wire 			        databus_eng_req;
   wire [`DATA_W-1:0] databus_data_from_eng;


   // PROGRAM MEMORY/CONTROLLER INTERFACE
   wire [`INSTR_W-1:0]      instruction;
   wire                     instr_valid;
   wire [`PROG_ADDR_W-1:0] 	pc;


   // DATA ENGINE/CONF MODULE INTERFACE
   wire [`CONF_BITS-1:0] 	  config_bus;

     //
     // SUBMODULES
     //
`ifdef DEBUG
   xprint cprint (
                .clk(clk),
                .sel(prt_req),
                .we(~ctr_rnw),
                .data_in(ctr_data_to_wr[7:0])
                );
`endif


   // databus address decoder (DATA I/F ADDR DECODER)

   xdatabus_addr_decoder databus_addr_decoder (
				       .clk(clk),
				       .rst(rst),

       				 // request, address and data to databus
				       .req(databus_req),
				       .addr(databus_addr),
				       .databus_data_out(databus_data_out),

				       //data engine
				       .eng_req(databus_eng_req),
				       .eng_data_from(databus_data_from_eng)
				       );

   // the following 2 modules to be combined in the Versat module
   // CONFIGURATION MODULE
   xconf conf (
	       .clk(clk),
	       .rst(rst),

	       // Control bus interface
	       .ctr_req(conf_req),
	       .ctr_rnw(ctr_rnw),
	       .ctr_addr(ctr_addr[`CONF_REG_ADDR_W-1:0]),
	       .ctr_data(ctr_data_to_wr[`DADDR_W-1:0]),

	       // configuration for engine
    	   .conf_out(config_bus)
	       );

   // DATA ENGINE
   xdata_eng data_eng (
		       .clk(clk),
		       .rst(rst),

		       // Controller interface
		       .ctr_req(eng_req),
		       .run_req(run_req),
           .ctr_rnw(ctr_rnw),
		       .ctr_addr(ctr_addr[`ENG_ADDR_W-1:0]),
		       .ctr_data_to_wr(ctr_data_to_wr),
		       .ctr_data_to_rd(eng_data_to_rd),

		       // databus interface
 		       .databus_req(databus_eng_req),
 		       .databus_rnw(databus_rnw),
 		       .databus_addr(databus_addr[`ENG_ADDR_W-1:0]),
		       .databus_data_in(databus_data_in),
	 	       .databus_data_out(databus_data_from_eng),

		       // Configuration bus input
		       .config_bus(config_bus),
           
           // Databus that comes from previous Versat 
           .data_bus_prev(databus_data_prev),
            
           // Databus to connect to next Versat 
           .data_bus(databus_data_next)
		       );

    xaddr_decoder addr_decoder (
	            .clk(clk),
	            .rst(rst),
                   //data engine
                   .eng_req(eng_req),
                   .eng_data_to_rd(eng_data_to_rd),

                   //configuration module (write only)
                   .conf_req(conf_req),

                   // bus request, address and data to read
                   .ctr_rnw(ctr_rnw),
                   .ctr_req(ctr_req),
                   .ctr_addr(ctr_addr),
                   .data_to_rd(ctr_data_to_rd),
                   .data_to_wr(ctr_data_to_wr)
                   );
endmodule
