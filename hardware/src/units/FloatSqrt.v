`timescale 1ns / 1ps

module FloatSqrt #(
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         running,
    input                         run,
    
    //input / output data
    input [DATA_W-1:0]            in0,

    input [31:0]                  delay0,
    
    (* versat_latency = 31 *) output [DATA_W-1:0]       out0
    );

reg start;
reg [31:0] delay;
wire done;

always @(posedge clk,posedge rst)
begin
     if(rst) begin
          start <= 1'b0;
          delay <= 0;
     end else if(run) begin
          delay <= delay0;
          start <= 1'b1;
     end else begin
          start <= 1'b0;
          if(|delay == 0) begin
               delay <= 0;
          end else begin
               delay <= delay - 1;
               if(delay == 1) begin
                    start <= 1'b1;
               end
          end
     end
end

iob_fp_sqrt sqrt(
     .start_i(start),
     .done_o(done),

     .op_i(in0),

     .res_o(out0),

     .overflow_o(),
     .underflow_o(),
     .exception_o(),

     .clk_i(clk),
     .rst_i(rst)
     );

endmodule
