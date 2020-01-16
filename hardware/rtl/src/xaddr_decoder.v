`timescale 1ns / 1ps
`include "xdefs.vh"
`include "xregfdefs.vh"
`include "xdata_engdefs.vh"
`include "xconfdefs.vh"
`include "xdivdefs.vh"

module xaddr_decoder (
		      // data engine
		      output reg 	       eng_req,
		      input [`DATA_W-1:0]      eng_data_to_rd,

		      // configuration module (write only)
		      output reg 	       conf_req,

		      // bus request, address and data to be read
		      input 		       clk,
		      input 		       rst,
		      input 		       ctr_req,
		      input 		       ctr_rnw,
		      input [`DATA_W-1:0]      data_to_wr,
		      input [`ADDR_W-1:0]      ctr_addr,
		      output reg [`DATA_W-1:0] data_to_rd
		      );

   //Signals
   reg [31:0]         dummy_reg;
   reg                dummy_reg_en;

   //compute requests
   always @ * begin
      conf_req = 1'b0;
      eng_req = 1'b0;
      dummy_reg_en = 0;

      if (`DUMMY_REG == (ctr_addr & ({`ADDR_W{1'b1}}<<`CONF_ADDR_W))) begin
         dummy_reg_en = ctr_req & ~ctr_rnw;
      end else if (`CONF_BASE == (ctr_addr & ({`ADDR_W{1'b1}}<<`CONF_ADDR_W))) begin
	       conf_req = ctr_req;
      end else if (`ENG_BASE == (ctr_addr & ({`ADDR_W{1'b1}}<<(`ENG_ADDR_W)))) begin
         eng_req = ctr_req;
      end else if (`ADDR_W'h0000 == ctr_addr) begin
         conf_req = ctr_req;
      end
   end // always @ *

   //compute data
   always @ * begin
      data_to_rd = `DADDR_W'd0;
      
      if (`DUMMY_REG == (ctr_addr & ({`ADDR_W{1'b1}}<<`CONF_ADDR_W))) begin
        data_to_rd = dummy_reg;
      end else if (`CONF_BASE == (ctr_addr & ({`ADDR_W{1'b1}}<<`CONF_ADDR_W))) begin
     data_to_rd = `DATA_W'd0;
      end else if (`ENG_BASE == (ctr_addr & ({`ADDR_W{1'b1}}<<(`ENG_ADDR_W)))) begin
         data_to_rd = eng_data_to_rd;
      end
   end
   

   //dummy reg
   always @(posedge clk)
     if(rst)
       dummy_reg <= 32'b0;
     else if(dummy_reg_en)
       dummy_reg <= data_to_wr;
endmodule
