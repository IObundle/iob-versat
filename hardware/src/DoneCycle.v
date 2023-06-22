`timescale 1ns / 1ps
`include "xversat.vh"

module DoneCycle #(
         parameter DATA_W = 32
              )
    (
    //control
    input                clk,
    input                rst,
    
    input                running,
    input                run,
    output               done,

    input [31:0]         amount
);

reg [31:0] counter;

assign done = (counter == 0);

always @(posedge clk,posedge rst)
begin
     if(rst) begin
          counter <= 0;
     end else if(run) begin
          counter <= amount;
     end else begin
          if(|counter) begin
               counter <= counter - 1;
          end
     end
end

endmodule
