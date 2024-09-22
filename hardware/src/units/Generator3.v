`timescale 1ns / 1ps

module Generator3 #(
   parameter PERIOD_W = 16,
   parameter DELAY_W = 7
) (
   input clk,
   input rst,

   input running,
   input run,

   //configurations 
   input [          31:0] iterations,
   input [          31:0] shift,

   input [PERIOD_W - 1:0] period,
   input [          31:0] incr,

   input [          31:0] iterations2,
   input [          31:0] shift2,

   input [PERIOD_W - 1:0] period2,
   input [          31:0] incr2,

   input [          31:0] iterations3,
   input [          31:0] shift3,

   input [PERIOD_W - 1:0] period3,
   input [          31:0] incr3,

   input [PERIOD_W - 1:0] duty,
   input [          31:0] start,

   input [DELAY_W-1:0] delay0,

   //outputs 
   (* versat_latency = 0 *)
   output [31:0] out0 // Latency zero because Address Gen is one cycle ahead in providing the address
);

   wire [31:0] genOut;
   wire        done;

   assign out0 = genOut;  //done ? off_value : genOut;

   MyAddressGen3 #(
      .ADDR_W  (32),
      .DATA_W  (8),
      .DELAY_W (DELAY_W),
      .PERIOD_W(PERIOD_W)
   ) addrGen (
      .clk_i(clk),
      .rst_i(rst),

      .run_i(run),

      //configurations 
      .iterations_i(iterations),
      .shift_i     (shift),

      .period_i    (period),
      .incr_i      (incr),

      .iterations2_i(iterations2),
      .shift2_i     (shift2),

      .period2_i    (period2),
      .incr2_i      (incr2),

      .iterations3_i(iterations3),
      .shift3_i     (shift3),

      .period3_i    (period3),
      .incr3_i      (incr3),

      .duty_i      (duty),
      .start_i     (start),

      .delay_i     (delay0),

      //outputs 
      .valid_o(),
      .ready_i(1'b1),
      .addr_o (genOut),

      .done_o(done)
   );

endmodule
