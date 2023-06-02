`timescale 1ns / 1ps

module xmux4 #(
         parameter DATA_W = 32
      )
    (
    //control
    input                         clk,
    input                         rst,
    input                         running,

    input                         run,
    output                        done,

    //input / output data
    input [DATA_W-1:0]            in0,
    input [DATA_W-1:0]            in1,
    input [DATA_W-1:0]            in2,
    input [DATA_W-1:0]            in3,

    output reg [DATA_W-1:0]       out0,
    
    input [1:0]                   sel
    );

assign done = 1'b1;

always @(posedge clk,posedge rst)
begin
   if(rst)
      out0 <= 0;
   else
      case(sel)
      2'b00: out0 <= in0;
      2'b01: out0 <= in1;
      2'b10: out0 <= in2;
      2'b11: out0 <= in3;
      endcase
end

endmodule