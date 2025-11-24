`timescale 1ns / 1ps

// Since Simple interface expects to write N transfers regardless of anything else (does not care about alignment or axi boundary)
// it becomes simpler to just have a module that performs N transfers from 
module TransferNFromSimpleM #(
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter LEN_W = 8,
   parameter MAX_TRANSF_W = 32
   ) (
      input [MAX_TRANSF_W-1:0] transferCount_i,
      input                    initiateTransfer_i,
      output                   done_o,

      // Connect directly to simple axi 
      input                        m_wvalid_i,
      output                       m_wready_o,
      input [      AXI_ADDR_W-1:0] m_waddr_i,
      input [      AXI_DATA_W-1:0] m_wdata_i,
      input [(AXI_DATA_W / 8)-1:0] m_wstrb_i,
      input [           LEN_W-1:0] m_wlen_i,
      output                       m_wlast_o,

      // Data output
      output                  data_valid_o,
      output [AXI_DATA_W-1:0] data_o,
      input                   data_ready_i,

      input rst_i,
      input clk_i
   );

reg [MAX_TRANSF_W-1:0] count;
reg working;

assign done_o = !working;
assign m_wready_o = working && data_ready_i;
assign data_valid_o = working && m_wvalid_i;
assign data_o = m_wdata_i;

assign m_wlast_o = (count == 1);

always @(posedge clk_i,posedge rst_i) begin
   if(rst_i) begin
      count <= 0;
      working <= 1'b0;
   end else if(working) begin
      if(m_wvalid_i && m_wready_o) begin
         count <= count - 1;

         if(count == 1) begin
            working <= 1'b0;
         end
      end
   end else if(initiateTransfer_i) begin
      count <= transferCount_i;
      working <= 1'b1;
   end
end

endmodule
