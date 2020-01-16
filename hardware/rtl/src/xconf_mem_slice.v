`timescale 1ns / 1ps
`include "xdefs.vh"

`ifdef CONF_MEM

module xconf_mem_slice (
			input 			 clk,
			input 			 en,

			// Flags
			input 			 databus_req,
			input 			 databus_rnw,
			input 			 conf_req,
			input 			 conf_rnw,

			// Adresses
			input [`CONF_ADDR_W-1:0] databus_addr,
			input [`CONF_ADDR_W-1:0] conf_addr,

			// Data in/out
			input [`DATA_W-1:0] 	 databus_data_in,
			output [`DATA_W-1:0] 	 databus_data_out,
			input [`DATA_W-1:0] 	 conf_data_in,
			output [`DATA_W-1:0] 	 conf_data_out
			);

`ifndef ASIC
   reg [`DATA_W-1:0] 				 conf_mem_slice [2**`CONF_ADDR_W-1:0];

   reg [`DATA_W-1:0] 				 databus_data_out_int;
   reg [`DATA_W-1:0] 				 conf_data_out_int;
`else
   wire 					 en_int;
   wire 					 we;
   
   wire [`CONF_ADDR_W-1:0] 			 addr;
   
   wire [`DATA_W-1:0] 				 data_in;
   wire [`DATA_W-1:0] 				 data_out;

   wire [`DATA_W-1:0] 				 databus_data_out_int;
   wire [`DATA_W-1:0] 				 conf_data_out_int;
`endif
   
   wire 					 en_databus;
   wire 					 en_conf;
   
   wire 					 wr_databus;
   wire 					 wr_conf;

   assign databus_data_out = databus_data_out_int;
   assign conf_data_out = conf_data_out_int;
   
`ifndef ALTERA // XILINX & ASIC
   assign en_databus = en & databus_req;
   assign en_conf = en & conf_req;
   
   assign wr_databus = ~databus_rnw;
   assign wr_conf = ~conf_rnw;
`else
   assign wr_databus = en & databus_req & ~databus_rnw;
   assign wr_conf = en & conf_req & ~conf_rnw;
`endif

`ifdef ASIC
   assign en_int = (databus_req)? en_databus : en_conf;
   assign we = (databus_req)? wr_databus : wr_conf;
   
   assign addr = (databus_req)? databus_addr : conf_addr;
   
   assign data_in = (databus_req)? databus_data_in : conf_data_in;
   
   assign databus_data_out_int = data_out;
   assign conf_data_out_int = data_out;
`endif
   
`ifdef XILINX
   always @ (posedge clk) begin : conf_reg_rw_databus
      if (en_databus) begin
	 if (wr_databus)
	   conf_mem_slice[databus_addr] <= databus_data_in;
	 databus_data_out_int <= conf_mem_slice[databus_addr];
      end
   end

   always @ (posedge clk) begin : conf_reg_rw
      if (en_conf) begin
	 if (wr_conf)
	   conf_mem_slice[conf_addr] <= conf_data_in;
	 conf_data_out_int <= conf_mem_slice[conf_addr];
      end
   end
`endif // !`ifdef XILINX
   
`ifdef ALTERA
   always @ (posedge clk) begin : conf_reg_rw_databus
      if (wr_databus)
	conf_mem_slice[databus_addr] <= databus_data_in;
      databus_data_out_int <= conf_mem_slice[databus_addr];
   end

   always @ (posedge clk) begin : conf_reg_rw
      if (wr_conf)
	conf_mem_slice[conf_addr] <= conf_data_in;
      conf_data_out_int <= conf_mem_slice[conf_addr];
   end
`endif // !`ifdef ALTERA
   
`ifdef ASIC
   
   xmem_wrapper64x32 memory_wrapper (
				     .clk(clk),
      
				     .en(en_int),
				     .wr(we),
				     .addr(addr),
				     .data_in(data_in),
				     .data_out(data_out)		
				     );
`endif //  `ifdef ASIC
   
endmodule
`endif
