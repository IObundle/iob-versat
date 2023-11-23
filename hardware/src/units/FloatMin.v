`timescale 1ns / 1ps

module FloatMin #(
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
    input [DATA_W-1:0]            in1,
    
    (* versat_latency = 1 *) output [DATA_W-1:0]       out0
    );

iob_fp_minmax min(
     .start_i(1'bx),
     .done_o(),

     .op_a_i(in0),
     .op_b_i(in1),

     .res_o(out0),

     .max_n_min_i(1'b0),

     .clk_i(clk),
     .rst_i(rst)
     );

endmodule
