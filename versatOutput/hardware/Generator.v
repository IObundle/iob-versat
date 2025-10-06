`timescale 1ns / 1ps

module Generator #(
   parameter PERIOD_W = 16,
   parameter DELAY_W = 7
) (
   input clk,
   input rst,

   input running,
   input run,

   //configurations 
   input [          31:0] iter,
   input [          31:0] shift,

   input [PERIOD_W - 1:0] per,
   input [          31:0] incr,

   input [          31:0] iter2,
   input [          31:0] shift2,

   input [PERIOD_W - 1:0] per2,
   input [          31:0] incr2,

   input [          31:0] iter3,
   input [          31:0] shift3,

   input [PERIOD_W - 1:0] per3,
   input [          31:0] incr3,

   input [PERIOD_W - 1:0] duty,
   input [          31:0] start,

   input [DELAY_W-1:0] delay0,

   //outputs 
   (* versat_latency = 0 *)
   output [31:0] out0 // Latency zero because Address Gen is one cycle ahead in providing the address
);

   wire [31:0] genOut;

   assign out0 = genOut;  //done ? off_value : genOut;

   AddressGen3 #(
      .ADDR_W  (32),
      .DATA_W  (8),
      .DELAY_W (DELAY_W),
      .PERIOD_W(PERIOD_W)
   ) addrGen (
      .clk_i(clk),
      .rst_i(rst),

      .run_i(run),

      .ignore_first_i(1'b0),

      //configurations 
      .duty_i  (duty),
      .start_i (start),

      .per_i   (per),
      .incr_i  (incr),

      .iter_i  (iter),
      .shift_i (shift),

      .per2_i  (per2),
      .incr2_i (incr2),

      .iter2_i (iter2),
      .shift2_i(shift2),

      .per3_i  (per3),
      .incr3_i (incr3),

      .iter3_i (iter3),
      .shift3_i(shift3),

      .delay_i (delay0),

      //outputs 
      .valid_o (),
      .ready_i (running),
      .addr_o  (genOut),
      .store_o (),

      .done_o  ()
   );

endmodule
