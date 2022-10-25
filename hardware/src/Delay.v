`timescale 1ns / 1ps

module Delay #(
         parameter ADDR_W = 6,
         parameter DATA_W = 32
      )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,

    //input / output data
    input [DATA_W-1:0]            in0,

    (* latency=1 *) output reg [DATA_W-1:0] out0,
    
    input [ADDR_W-1:0]     amount
    );

wire [ADDR_W-1:0] occupancy;
wire read_en = (occupancy >= amount);
wire write_en = (occupancy <= amount);

reg [DATA_W-1:0] inData;
wire [DATA_W-1:0] fifo_data;
iob_sync_fifo #(.DATA_W(DATA_W),.ADDR_W(ADDR_W))
   fifo 
   (
      .rst(rst),
      .clk(clk),
   
      .fifo_ocupancy(occupancy), 

      //read port
      .r_data(fifo_data), 
      .r_empty(),
      .r_en(read_en),

      //write port
      .w_data(in0),
      .w_full(),
      .w_en(write_en)
   );

always @(posedge clk,posedge rst)
begin
   if(rst)
      inData <= 0;
   else
      inData <= in0;
end

always @*
begin
   out0 = 0;

   if(amount == 0)
      out0 = inData;
   else
      out0 = fifo_data;
end

endmodule