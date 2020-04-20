`timescale 1ns / 1ps
`include "xversat.vh"
`include "xmemdefs.vh"

module xmem #(
              parameter MEM_INIT_FILE="none"
              )
   (
    //control
    input                         clk,
    input                         rst,
    input                         initA,
    input                         initB,
    input                         runA,
    input                         runB,
    output                        doneA,
    output                        doneB,

    //controller interface
    input                         ctr_mem_valid,
    input                         ctr_we,
    input [`MEM_ADDR_W-1:0]       ctr_addr,
    input [`DATA_W-1:0]           ctr_data_in,

    //databus interface
    input                         data_we,
    input [`MEM_ADDR_W-1:0]       data_addr,
    input [`DATA_W-1:0]           data_data_in,
    input                         data_mem_valid,

    //input / output data
    input [2*`DATABUS_W-1:0]      flow_in,
    output [2*`DATA_W-1:0]        flow_out,

    //configurations
    input [2*`MEMP_CONF_BITS-1:0] config_bits
    );

   wire [`DATA_W-1:0]             outA;
   wire [`DATA_W-1:0]             outB;
   
   assign flow_out = {outA, outB};

   
   function [`MEM_ADDR_W-1:0] reverseBits;
      input [`MEM_ADDR_W-1:0]     word;
      integer                     i;

      begin
	 for (i=0; i < `MEM_ADDR_W; i=i+1)
	   reverseBits[i]=word[`MEM_ADDR_W-1 - i];
      end
   endfunction

   //unpack configuration bits 
   wire [`MEM_ADDR_W-1:0] iterationsA = config_bits[2*`MEMP_CONF_BITS-1 -: `MEM_ADDR_W];                                           
   wire [`PERIOD_W-1:0]   perA        = config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-1 -: `PERIOD_W];                                 
   wire [`PERIOD_W-1:0]   dutyA       = config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-`PERIOD_W-1 -: `PERIOD_W];                       
   wire [`N_W-1:0]        selA        = config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-1 -: `N_W];                          
   wire [`MEM_ADDR_W-1:0] startA      = config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-1 -: `MEM_ADDR_W];              
   wire [`MEM_ADDR_W-1:0] shiftA      = config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-`MEM_ADDR_W-1 -: `MEM_ADDR_W];  
   wire [`MEM_ADDR_W-1:0] incrA       = config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-2*`MEM_ADDR_W-1 -: `MEM_ADDR_W];
   wire [`PERIOD_W-1:0]   delayA      = config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-1 -: `PERIOD_W];  
   wire                   reverseA    = config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-`PERIOD_W-1 -: 1];
   wire                   extA        = config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-`PERIOD_W-1-1 -: 1];
   wire                   inA_wr      = config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-`PERIOD_W-1-1-1 -: 1];
   wire                   addrA_out_en= config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-`PERIOD_W-1-1-1-1 -: 1];

   wire [`MEM_ADDR_W-1:0] iterationsB = config_bits[`MEMP_CONF_BITS-1 -: `MEM_ADDR_W];                                             
   wire [`PERIOD_W-1:0]   perB        = config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-1 -: `PERIOD_W];                                   
   wire [`PERIOD_W-1:0]   dutyB       = config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-`PERIOD_W-1 -: `PERIOD_W];                         
   wire [`N_W-1:0]        selB        = config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-1 -: `N_W];                            
   wire [`MEM_ADDR_W-1:0] startB      = config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-1 -: `MEM_ADDR_W];                
   wire [`MEM_ADDR_W-1:0] shiftB      = config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-`MEM_ADDR_W-1 -: `MEM_ADDR_W];    
   wire [`MEM_ADDR_W-1:0] incrB       = config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-2*`MEM_ADDR_W-1 -: `MEM_ADDR_W];  
   wire [`PERIOD_W-1:0]   delayB      = config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-1 -: `PERIOD_W];    
   wire                   reverseB    = config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-`PERIOD_W-1 -: 1];  
   wire                   extB        = config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-`PERIOD_W-1-1 -: 1];
   wire                   inB_wr      = config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-`PERIOD_W-1-1-1 -: 1];
   wire                   addrB_out_en= config_bits[`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-`PERIOD_W-1-1-1-1 -: 1];

   // ports program ready
   wire readyA = |iterationsA;
   wire readyB = |iterationsB;

   //mem enables output by addr gen
   wire enA_int, enB_int;

   wire enA = enA_int|data_mem_valid;
   wire enB = enB_int|ctr_mem_valid;

   wire [`DATA_W-1:0]     outA_int, outB_int;

   wire wrA = data_mem_valid? data_we : (enA_int & inA_wr & ~extA); //addrgen on & input on & input isn't address
   wire wrB = ctr_mem_valid? ctr_we : (enB_int & inB_wr & ~extB);


   //port addresses and enables
   wire [`MEM_ADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [`MEM_ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   assign outA = addrA_out_en? {{(`DATA_W-`MEM_ADDR_W){addrA_int2[`MEM_ADDR_W-1]}}, addrA_int2} : outA_int;
   assign outB = addrB_out_en? {{(`DATA_W-`MEM_ADDR_W){addrB_int2[`MEM_ADDR_W-1]}}, addrB_int2}: outB_int;

   //data inputs
   wire [`DATA_W-1:0]     inA;
   wire [`DATA_W-1:0]     inB;

   //input selection
   xinmux muxa (
		.sel(selA),
		.data_in(flow_in),
		.data_out(inA)
		);

   xinmux  muxb (
		 .sel(selB),
		 .data_in(flow_in),
		 .data_out(inB)
		 );

   wire [`DATA_W-1:0]     data_to_wrA = data_mem_valid? data_data_in : inA ;
   wire [`DATA_W-1:0]     data_to_wrB  = ctr_mem_valid?  ctr_data_in : inB;

   //address generators
   xaddrgen addrgenA (
		      .clk(clk),
		      .rst(rst),
		      .init(initA),
		      .run(runA & readyA),
		      .iterations(iterationsA),
		      .period(perA),
		      .duty(dutyA),
		      .start(startA),
		      .shift(shiftA),
		      .incr(incrA),
		      .delay(delayA),
		      .addr(addrA_int),
		      .mem_en(enA_int),
		      .done(doneA)
		      );

   xaddrgen addrgenB (
		      .clk(clk),
		      .rst(rst),
		      .init(initB),
		      .run(runB & readyB),
		      .iterations(iterationsB),
		      .period(perB),
		      .duty(dutyB),
		      .start(startB),
		      .shift(shiftB),
		      .incr(incrB),
		      .delay(delayB),
		      .addr(addrB_int),
		      .mem_en(enB_int),
		      .done(doneB)
		      );

   assign addrA = data_mem_valid? data_addr[`MEM_ADDR_W-1:0] : extB? inB[`MEM_ADDR_W-1:0] : addrA_int2[`MEM_ADDR_W-1:0];
   assign addrB = ctr_mem_valid? ctr_addr : extA? inA[`MEM_ADDR_W-1:0] : addrB_int2[`MEM_ADDR_W-1:0];

   assign addrA_int2 = reverseA? reverseBits(addrA_int) : addrA_int;
   assign addrB_int2 = reverseB? reverseBits(addrB_int) : addrB_int;

   iob_t2p_mem #(
                   .MEM_INIT_FILE(MEM_INIT_FILE),
		   .DATA_W(`DATA_W),
		   .ADDR_W(`MEM_ADDR_W))
   mem
     (
      .data_a(data_to_wrA),
      .data_b(data_to_wrB),
      .addr_a(addrA[`MEM_ADDR_W-1:0]),
      .addr_b(addrB[`MEM_ADDR_W-1:0]),
      .en_a(enA),
      .en_b(enB),
      .we_a(wrA),
      .we_b(wrB),
      .q_a(outA_int),
      .q_b(outB_int),
      .clk(clk)
      );

endmodule
