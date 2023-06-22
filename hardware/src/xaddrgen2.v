`timescale 1ns / 1ps
`include "xversat.vh"

module xaddrgen2 # ( 
       parameter           MEM_ADDR_W = `MEM_ADDR_W,
       parameter           PERIOD_W = `PERIOD_W
      ) (
       input                                  clk,
       input                                  rst,
       input                                  run,

       //configurations
       input [MEM_ADDR_W - 1:0]              iterations,
       input [PERIOD_W - 1:0]                period,
       input [PERIOD_W - 1:0]                duty,
       input [PERIOD_W - 1:0]                delay,
       input [MEM_ADDR_W - 1:0]              start,
       input signed [MEM_ADDR_W - 1:0]       shift,
       input signed [MEM_ADDR_W - 1:0]       incr,
       input [MEM_ADDR_W - 1:0]              iterations2,
       input [PERIOD_W - 1:0]                period2,
       input signed [MEM_ADDR_W - 1:0]       shift2,
       input signed [MEM_ADDR_W - 1:0]       incr2,

       //outputs
       output [MEM_ADDR_W - 1:0]             addr,
       output                                mem_en,
       output                                done
       );

   //connection wires
   wire [MEM_ADDR_W - 1:0]                             addrB;
   wire                                                 mem_enB;
   wire                                                 doneA, doneB;
   wire                                                 mem_en_reg_w;

   //only run if iterations > 0
   wire                                                 readyA = |iterations;
   wire                                                 readyB = |iterations2;

   //keep running addrgenA while addrgenB is enabled
   wire                                                 runA = run | mem_enB;

   //done if both addrgen are done
   assign                                               done = doneA & (doneB | ~readyB);

   //update addrgenB after addrgenA is done
   wire                                                 pause = mem_en_reg_w & ~doneA;

   //after first run, addrgenA start comes from addrgenB addr
   wire  [MEM_ADDR_W - 1:0]            startA = run ? start : addrB;

   //instantiate address generators
   xaddrgen # (
   .MEM_ADDR_W(MEM_ADDR_W),
   .PERIOD_W(PERIOD_W)
   ) addrgenA (
        .clk(clk),
        .rst(rst),
        .init(runA),
        .run(runA & readyA),
        .pause(1'b0),
        .iterations(iterations),
        .period(period),
        .duty(duty),
        .delay(delay),
        .start(startA),
        .shift(shift),
        .incr(incr),
        .addr(addr),
        .mem_en(mem_en),
        .done(doneA)
        );

   xaddrgen # ( 
   .MEM_ADDR_W(MEM_ADDR_W),
   .PERIOD_W(PERIOD_W)
   ) addrgenB (
        .clk(clk),
        .rst(rst),
        .init(run),
        .run(run & readyB),
        .pause(pause),
        .iterations(iterations2),
        .period(period2),
        .duty(period2),
        .delay(delay),
        .start(start),
        .shift(shift2),
        .incr(incr2),
        .addr(addrB),
        .mem_en(mem_enB),
        .done(doneB)
        );

   //register mem_en of addrgenA
   reg mem_en_reg;
   always @ (posedge clk, posedge rst)
     if(rst)
       mem_en_reg <= 1'b0;
     else
       mem_en_reg <= mem_en;
   assign mem_en_reg_w = mem_en_reg;

endmodule
