`timescale 1ns / 1ps

module MemoryWriter #(
   parameter ADDR_W = 32,
   parameter DATA_W = 32
   )(
      // Slave connected to address gen
      input gen_valid,
      output reg gen_ready,
      input [ADDR_W-1:0] gen_addr,

      // Slave connected to data source
      input data_valid,
      output reg data_ready,
      input [DATA_W-1:0] data_in,

      // Connect to memory
      output reg              mem_enable,
      output reg [ADDR_W-1:0] mem_addr,
      output reg [DATA_W-1:0] mem_data,

      input clk,
      input rst
   );

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      mem_enable <= 0;
      mem_addr <= 0;
      mem_data <= 0;
   end else begin
      mem_enable <= 1'b0;

      if(gen_valid && data_valid) begin
         mem_addr <= gen_addr;
         mem_data <= data_in;
         mem_enable <= 1'b1; 
      end
   end
end

always @*
begin
   gen_ready = 1'b0;
   data_ready = 1'b0;

   data_ready = gen_valid;

   if(data_valid) begin
      gen_ready = 1'b1;
   end
end

endmodule




