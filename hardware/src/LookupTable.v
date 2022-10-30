`timescale 1ns / 1ps

module LookupTable #(
       parameter INIT_MEM_FILE="none",
       parameter DATA_W = 32,
       parameter ADDR_W = 8
   )
   (
      //databus interface
      input [DATA_W/8-1:0]          wstrb,
      input [ADDR_W-1:0]            addr,
      input [DATA_W-1:0]            wdata,
      input                         valid,
      output reg                    ready,
      output [DATA_W-1:0]           rdata,

      input [DATA_W-1:0] in0,
      input [DATA_W-1:0] in1,

      (* versat_latency = 2 *) output reg [DATA_W-1:0] out0,
      (* versat_latency = 2 *) output reg [DATA_W-1:0] out1,

      input clk,
      input rst,
      input run
   );

   reg [ADDR_W-1:0] addr_reg;
   reg [DATA_W-1:0] data;
   reg write;

   always @(posedge clk,posedge rst)
   begin
      if(rst) begin
         data <= 0;
         addr_reg <= 0;
         write <= 1'b0;
         ready <= 1'b0;
      end else begin
         write <= 1'b0;
         ready <= 1'b0;

         if(valid && |wstrb) begin
            data <= wdata;
            addr_reg <= addr;
            write <= 1'b1;
            ready <= 1'b1;
         end
      end
   end

   wire [DATA_W-1:0] outA,outB;

   always @(posedge clk,posedge rst)
   begin
      if(rst) begin
         out0 <= 0;
         out1 <= 0;
      end else begin
         out0 <= outA;
         out1 <= outB;
      end
   end

   iob_dp_ram #(
         .FILE(INIT_MEM_FILE),
         .DATA_W(DATA_W),
         .ADDR_W(ADDR_W))
   mem
     (
      .dinA(data),
      .dinB(0),
      .addrA(write ? addr_reg : in0[ADDR_W-1:0]),
      .addrB(in1[ADDR_W-1:0]),
      .enA(1'b1),
      .enB(1'b1),
      .weA(write),
      .weB(1'b0),
      .doutA(outA),
      .doutB(outB),
      .clk(clk)
      );

endmodule
