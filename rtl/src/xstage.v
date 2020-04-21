`timescale 1ns / 1ps

// top defines
`include "xversat.vh"
`include "xconfdefs.vh"

// FU defines
`include "xmemdefs.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"

module xstage
  (
   input                        	clk,
   input                        	rst,
   
   //control interface
   input                        	ctr_valid,
   input                        	ctr_we,
   input [`nMEM_W+`MEM_ADDR_W+1:0] 	ctr_addr,
   input [`DATA_W-1:0]          	ctr_data_in,
   output [`DATA_W-1:0]         	ctr_data_out,

   //data slave interface
   input                        	data_valid,
   input                        	data_we,
   input [`nMEM_W+`MEM_ADDR_W-1:0]     	data_addr,
   input [`DATA_W-1:0]          	data_data_in,
   output [`DATA_W-1:0]         	data_data_out,
  
   //versat flow interface      
   input [`DATABUS_W-1:0]       	flow_in, 
   output [`DATABUS_W-1:0]      	flow_out
   );
   
   //simple address decode
   wire                       		conf_valid = ctr_addr[`nMEM_W+`MEM_ADDR_W+1] & ctr_valid;
   wire					eng_valid = ~ctr_addr[`nMEM_W+`MEM_ADDR_W+1] & ctr_valid;

   //configuration bus from conf module to versat
   wire [`CONF_BITS-1:0]   		config_bus;
   
   //configuration module
   xconf conf (
	       .clk(clk),
	       .rst(rst),

	       // Control bus interface
	       .ctr_valid(conf_valid),
	       .ctr_we(ctr_we),
	       .ctr_addr(ctr_addr[`CONF_REG_ADDR_W:0]),
	       .ctr_data_in(ctr_data_in[`MEM_ADDR_W-1:0]),

	       // configuration for engine
    	       .conf_out(config_bus)
	       );

   //data engine
   xdata_eng data_eng (
		       .clk(clk),
		       .rst(rst),

	               // control interface
	               .ctr_valid(eng_valid),
                       .ctr_we(ctr_we),
		       .ctr_addr(ctr_addr[`nMEM_W+`MEM_ADDR_W:0]),
		       .ctr_data_in(ctr_data_in),
		       .ctr_data_out(ctr_data_out),

                       // data interface
 		       .data_valid(data_valid),
 		       .data_we(data_we),
 		       .data_addr(data_addr),
		       .data_data_in(data_data_in),
	 	       .data_data_out(data_data_out),

                       //flow interface
                       .flow_in(flow_in),
                       .flow_out(flow_out),

                       // configuration bus
		       .config_bus(config_bus)
		       );
 
endmodule
