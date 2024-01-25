`timescale 1ns / 1ps

module xaddrgen2 #(
   parameter MEM_ADDR_W = 10,
   parameter PERIOD_W   = 10
) (
   input clk_i,
   input rst_i,
   input run_i,

   //configurations
   input        [MEM_ADDR_W - 1:0] iterations_i,
   input        [  PERIOD_W - 1:0] period_i,
   input        [  PERIOD_W - 1:0] duty_i,
   input        [  PERIOD_W - 1:0] delay_i,
   input        [MEM_ADDR_W - 1:0] start_i,
   input signed [MEM_ADDR_W - 1:0] shift_i,
   input signed [MEM_ADDR_W - 1:0] incr_i,
   input        [MEM_ADDR_W - 1:0] iterations2_i,
   input        [  PERIOD_W - 1:0] period2_i,
   input signed [MEM_ADDR_W - 1:0] shift2_i,
   input signed [MEM_ADDR_W - 1:0] incr2_i,

   //outputs
   output [MEM_ADDR_W - 1:0] addr_o,
   output                    mem_en_o,
   output                    done_o
);

   //connection wires
   wire [MEM_ADDR_W - 1:0] addr_oB;
   wire                      mem_en_oB;
   wire done_oA, done_oB;
   wire                             mem_en_o_reg_w;

   //only run_i if iterations_i > 0
   wire readyA = |iterations_i;
   wire readyB = |iterations2_i;

   //keep run_ining addr_ogenA while addr_ogenB is enabled
   wire run_iA = run_i | mem_en_oB;

   //done_o if both addr_ogen are done_o
   assign done_o = done_oA & (done_oB | ~readyB);

   //update addr_ogenB after addr_ogenA is done_o
   wire pause = mem_en_o_reg_w & ~done_oA;

   //after first_i run_i, addr_ogenA start_i comes from addr_ogenB addr_o
   wire [MEM_ADDR_W - 1:0] start_iA = run_i ? start_i : addr_oB;

   //instantiate addr_oess generators
   xaddrgen #(
      .MEM_ADDR_W(MEM_ADDR_W),
      .PERIOD_W  (PERIOD_W)
   ) addrgenA (
      .clk_i       (clk_i),
      .rst_i       (rst_i),
      .init        (run_iA),
      .run_i       (run_iA & readyA),
      .pause       (1'b0),
      .iterations_i(iterations_i),
      .period_i    (period_i),
      .duty_i      (duty_i),
      .delay_i     (delay_i),
      .start_i     (start_iA),
      .shift_i     (shift_i),
      .incr_i      (incr_i),
      .addr_o      (addr_o),
      .mem_en_o    (mem_en_o),
      .done_o      (done_oA)
   );

   xaddrgen #(
      .MEM_ADDR_W(MEM_ADDR_W),
      .PERIOD_W  (PERIOD_W)
   ) addrgenB (
      .clk_i       (clk_i),
      .rst_i       (rst_i),
      .init        (run_i),
      .run_i       (run_i & readyB),
      .pause       (pause),
      .iterations_i(iterations2_i),
      .period_i    (period2_i),
      .duty_i      (period2_i),
      .delay_i     (delay_i),
      .start_i     (start_i),
      .shift_i     (shift2_i),
      .incr_i      (incr2_i),
      .addr_o      (addr_oB),
      .mem_en_o    (mem_en_oB),
      .done_o      (done_oB)
   );

   //register mem_en_o of addr_ogenA
   reg mem_en_o_reg;
   always @(posedge clk_i, posedge rst_i)
      if (rst_i) mem_en_o_reg <= 1'b0;
      else mem_en_o_reg <= mem_en_o;
   assign mem_en_o_reg_w = mem_en_o_reg;

endmodule  // xaddrgen2
