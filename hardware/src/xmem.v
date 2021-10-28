`timescale 1ns / 1ps
`include "xversat.vh"
`include "xmemdefs.vh"

module xmem #(
              parameter MEM_INIT_FILE="none",
	      parameter DATA_W = 32,
         parameter ADDR_W = 16
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,
    output                        done,

    //mem interface
    input                         we,
    input [ADDR_W-1:0]            addr,
    input [DATA_W-1:0]            wdata,
    input                         valid,
    output reg                    ready,
    output [DATA_W-1:0]           rdata,

    //input / output data
    input [DATA_W-1:0]            in0,
    input [DATA_W-1:0]            in1,
    output [DATA_W-1:0]           out0,
    output [DATA_W-1:0]           out1,

    //configurations
    input [2*`MEMP_CONF_BITS-1:0] configdata
    );

   wire doneA,doneB;
   assign done = doneA & doneB;

   //output databus
   wire [DATA_W-1:0]              outA, outB;
   reg  [DATA_W-1:0]              outA_reg, outB_reg;
   
   //assign flow_out =            {outA_reg, outB_reg};
   assign out0 = outA_reg;
   assign out1 = 0;

   //function to reverse bits
   function [ADDR_W-1:0] reverseBits;
      input [ADDR_W-1:0]     word;
      integer                     i;

      begin
	 for (i=0; i < ADDR_W; i=i+1)
	   reverseBits[i]=word[ADDR_W-1 - i];
      end
   endfunction

   //unpack configuration bits 
   wire [ADDR_W-1:0]    iterA    = configdata[2*`MEMP_CONF_BITS-1 -: ADDR_W];
   wire [`PERIOD_W-1:0] perA     = configdata[2*`MEMP_CONF_BITS-ADDR_W-1 -: `PERIOD_W];
   wire [`PERIOD_W-1:0] dutyA    = configdata[2*`MEMP_CONF_BITS-ADDR_W-`PERIOD_W-1 -: `PERIOD_W];
   wire [ADDR_W-1:0]    startA   = configdata[2*`MEMP_CONF_BITS-ADDR_W-2*`PERIOD_W-1 -: ADDR_W];
   wire [ADDR_W-1:0]    shiftA   = configdata[2*`MEMP_CONF_BITS-2*ADDR_W-2*`PERIOD_W-1 -: ADDR_W];
   wire [ADDR_W-1:0]    incrA    = configdata[2*`MEMP_CONF_BITS-3*ADDR_W-2*`PERIOD_W-1 -: ADDR_W];
   wire [`PERIOD_W-1:0] delayA   = configdata[2*`MEMP_CONF_BITS-4*ADDR_W-2*`PERIOD_W-1 -: `PERIOD_W];
   wire                 reverseA = configdata[2*`MEMP_CONF_BITS-4*ADDR_W-3*`PERIOD_W-1 -: 1];
   wire                 extA     = configdata[2*`MEMP_CONF_BITS-4*ADDR_W-3*`PERIOD_W-1-1 -: 1];
   wire                 in0_wr   = configdata[2*`MEMP_CONF_BITS-4*ADDR_W-3*`PERIOD_W-2-1 -: 1];
   wire [ADDR_W-1:0]    iter2A   = configdata[2*`MEMP_CONF_BITS-4*ADDR_W-3*`PERIOD_W-3-1 -: ADDR_W];
   wire [`PERIOD_W-1:0] per2A    = configdata[2*`MEMP_CONF_BITS-5*ADDR_W-3*`PERIOD_W-3-1 -: `PERIOD_W];
   wire [ADDR_W-1:0]    shift2A  = configdata[2*`MEMP_CONF_BITS-5*ADDR_W-4*`PERIOD_W-3-1 -: ADDR_W];
   wire [ADDR_W-1:0]    incr2A   = configdata[2*`MEMP_CONF_BITS-6*ADDR_W-4*`PERIOD_W-3-1 -: ADDR_W];

   wire [ADDR_W-1:0]    iterB    = configdata[`MEMP_CONF_BITS-1 -: ADDR_W];
   wire [`PERIOD_W-1:0] perB     = configdata[`MEMP_CONF_BITS-ADDR_W-1 -: `PERIOD_W];
   wire [`PERIOD_W-1:0] dutyB    = configdata[`MEMP_CONF_BITS-ADDR_W-`PERIOD_W-1 -: `PERIOD_W];
   wire [ADDR_W-1:0]    startB   = configdata[`MEMP_CONF_BITS-ADDR_W-2*`PERIOD_W-1 -: ADDR_W];
   wire [ADDR_W-1:0]    shiftB   = configdata[`MEMP_CONF_BITS-2*ADDR_W-2*`PERIOD_W-1 -: ADDR_W];
   wire [ADDR_W-1:0]    incrB    = configdata[`MEMP_CONF_BITS-3*ADDR_W-2*`PERIOD_W-1 -: ADDR_W];
   wire [`PERIOD_W-1:0] delayB   = configdata[`MEMP_CONF_BITS-4*ADDR_W-2*`PERIOD_W-1 -: `PERIOD_W];
   wire                 reverseB = configdata[`MEMP_CONF_BITS-4*ADDR_W-3*`PERIOD_W-1 -: 1];
   wire                 extB     = configdata[`MEMP_CONF_BITS-4*ADDR_W-3*`PERIOD_W-1-1 -: 1];
   wire                 in1_wr   = configdata[`MEMP_CONF_BITS-4*ADDR_W-3*`PERIOD_W-2-1 -: 1];
   wire [ADDR_W-1:0]    iter2B   = configdata[`MEMP_CONF_BITS-4*ADDR_W-3*`PERIOD_W-3-1 -: ADDR_W];
   wire [`PERIOD_W-1:0] per2B    = configdata[`MEMP_CONF_BITS-5*ADDR_W-3*`PERIOD_W-3-1 -: `PERIOD_W];
   wire [ADDR_W-1:0]    shift2B  = configdata[`MEMP_CONF_BITS-5*ADDR_W-4*`PERIOD_W-3-1 -: ADDR_W];
   wire [ADDR_W-1:0]    incr2B   = configdata[`MEMP_CONF_BITS-6*ADDR_W-4*`PERIOD_W-3-1 -: ADDR_W];

   //mem enables output by addr gen
   wire enA_int;
   wire enA = enA_int | valid;
   wire enB;

   //write enables
   wire wrA = valid? we : (enA_int & in0_wr & ~extA); //addrgen on & input on & input isn't address
   wire wrB = (enB & in1_wr & ~extB);

   //port addresses and enables
   wire [ADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   //data inputs
   wire [DATA_W-1:0]     in0;
   wire [DATA_W-1:0]     in1;

   wire [DATA_W-1:0]     data_to_wrA = valid? wdata : in0 ;

   //address generators
   xaddrgen2 addrgen2A (
		      .clk(clk),
		      .rst(rst),
		      .run(run),
		      .iterations(iterA),
		      .period(perA),
		      .duty(dutyA),
		      .start(startA),
		      .shift(shiftA),
		      .incr(incrA),
		      .delay(delayA),
		      .iterations2(iter2A),
                      .period2(per2A),
                      .shift2(shift2A),
                      .incr2(incr2A),
		      .addr(addrA_int),
		      .mem_en(enA_int),
		      .done(doneA)
		      );

   xaddrgen2 addrgen2B (
		      .clk(clk),
		      .rst(rst),
		      .run(run),
		      .iterations(iterB),
		      .period(perB),
		      .duty(dutyB),
		      .start(startB),
		      .shift(shiftB),
		      .incr(incrB),
		      .delay(delayB),
		      .iterations2(iter2B),
                      .period2(per2B),
                      .shift2(shift2B),
                      .incr2(incr2B),
		      .addr(addrB_int),
		      .mem_en(enB),
		      .done(doneB)
		      );

   //define addresses based on ext and rvrs
   assign addrA = valid? addr[ADDR_W-1:0] : extA? in0[ADDR_W-1:0] : addrA_int2[ADDR_W-1:0];
   assign addrB = extB? in1[ADDR_W-1:0] : addrB_int2[ADDR_W-1:0];
   assign addrA_int2 = reverseA? reverseBits(addrA_int) : addrA_int;
   assign addrB_int2 = reverseB? reverseBits(addrB_int) : addrB_int;

   //register mem inputs
   reg [DATA_W-1:0] data_a_reg, data_b_reg;
   reg [ADDR_W-1:0] addr_a_reg, addr_b_reg;
   reg en_a_reg, en_b_reg, we_a_reg, we_b_reg;
   always @ (posedge clk) begin
      data_a_reg <= data_to_wrA;
      data_b_reg <= in1;
      addr_a_reg <= addrA;
      addr_b_reg <= addrB;
      en_a_reg <= enA;
      en_b_reg <= enB;
      we_a_reg <= wrA;
      we_b_reg <= wrB;
   end

   // Read 
   assign rdata = (ready ? outA_reg : 32'h0);

   // Delay by a cycle to match memory read latency
   reg readCounter[1:0];
   always @(posedge clk,posedge rst)
   begin
      if(rst) begin
         ready <= 1'b0;
         readCounter[0] <= 1'b0;
         readCounter[1] <= 1'b0;
      end else begin
         if(ready) begin
            ready <= 1'b0;
            readCounter[0] <= 1'b0;
            readCounter[1] <= 1'b0;
         end else begin
            // Write
            if(valid && we)
               ready <= 1'b1;

            // Read
            ready <= readCounter[0];
            readCounter[0] <= readCounter[1];
            readCounter[1] <= valid;
         end
      end
   end

   iob_tdp_ram #(
                   .MEM_INIT_FILE(MEM_INIT_FILE),
		   .DATA_W(DATA_W),
		   .ADDR_W(ADDR_W))
   mem
     (
      .data_a(data_a_reg),
      .data_b(data_b_reg),
      .addr_a(addr_a_reg),
      .addr_b(addr_b_reg),
      .en_a(en_a_reg),
      .en_b(en_b_reg),
      .we_a(we_a_reg),
      .we_b(we_b_reg),
      .q_a(outA),
      .q_b(outB),
      .clk(clk)
      );

   //register mem outputs
   always @ (posedge clk) begin
      outA_reg <= outA;
      outB_reg <= outB;
   end

endmodule
