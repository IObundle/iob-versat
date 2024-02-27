`timescale 1ns / 1ps

module Generator #(
   parameter PERIOD_W = 16
) (
   input clk,
   input rst,

   input running,
   input run,

   //configurations 
   input [          31:0] iterations,
   input [PERIOD_W - 1:0] period,
   input [PERIOD_W - 1:0] duty,
   input [          31:0] start,
   input [          31:0] shift,
   input [          31:0] incr,

   input [31:0] delay0,

   //outputs 
   (* versat_latency = 0 *)
   output [31:0] out0 // Latency zero because Address Gen is one cycle ahead in providing the address
);

   wire [31:0] genOut;
   wire        done;

   assign out0 = genOut;  //done ? off_value : genOut;

   MyAddressGen #(
      .ADDR_W  (32),
      .DATA_W  (8),
      .PERIOD_W(PERIOD_W)
   ) addrGen (
      .clk_i(clk),
      .rst_i(rst),

      .run_i(run),

      //configurations 
      .iterations_i(iterations),
      .period_i    (period),
      .duty_i      (duty),
      .delay_i     (delay0),
      .start_i     (start),
      .shift_i     (shift),
      .incr_i      (incr),

      //outputs 
      .valid_o(),
      .ready_i(1'b1),
      .addr_o (genOut),

      .done_o(done)
   );

endmodule
