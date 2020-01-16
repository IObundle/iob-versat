`timescale 1ns / 1ps
`include "xdefs.vh"
`include "xprogdefs.vh"
`include "xdata_engdefs.vh"

module xdatabus_addr_decoder (
        input                     clk,
        input                     rst,
        
        // request, address and data to read
        input                     req,
        input [`ADDR_W-1:0]       addr,
        output reg [`DATA_W-1:0]  databus_data_out,

`ifdef PROG_MEM_USE
			  //prog mem
        output reg                prog_req,
        input [`DATA_W-1:0]       prog_data_from,
`endif
        //data engine
        output reg                eng_req,
        input [`DATA_W-1:0]       eng_data_from
        );

  reg [`ADDR_W-1:0]               addr_reg;
  reg                             req_reg;
   

   // databus address register: delay by 1 cycle to select right data
   always @ (posedge rst, posedge clk)
     if (rst) begin
        addr_reg <= `ADDR_W'd0;
        req_reg <= 1'b0;
     end else begin
        addr_reg <= addr;
        req_reg <= req;
     end

   //decode address and send request to addressed module
   always @* begin
     eng_req = 1'b0;

`ifdef PROG_MEM_USE
     prog_req = 1'b0;
`endif

     if (`ENG_BASE == (addr & ({`ADDR_W{1'b1}}<<(`ENG_ADDR_W))))
        eng_req = req;
      
`ifdef PROG_MEM_USE
     else if ( (`PROG_BASE+`PROG_MEM)  == (addr & ({`ADDR_W{1'b1}}<<`PROG_MEM_ADDR_W)))
        prog_req = req;
`endif

     else if (req)
        $display("Warning: unmapped databus issued address %x at time %f", addr, $time);
   end // always @*

   //select output of previously addressed module due to one cycle latency
   always @ * begin
     databus_data_out = eng_data_from;    
        if (`ENG_BASE == (addr_reg & ({`ADDR_W{1'b1}}<<`ENG_ADDR_W)))
          databus_data_out = eng_data_from;
`ifdef PROG_MEM_USE
        else if (`PROG_BASE == (addr_reg & ({`ADDR_W{1'b1}}<<`PROG_ADDR_W)))
	        databus_data_out = prog_data_from;
`endif
        else if (req_reg)
          $display("Warning: unmapped databus issued address %x at time %f", addr_reg, $time);
   end
endmodule
