`timescale 1ns / 1ps
`include "xversat.vh"

`include "xmemdefs.vh"

module xmem #(
              parameter MEM_INIT_FILE=0,
              parameter ADDR_W=11
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
    input                         ctr_mem_req,
    input                         ctr_rnw,
    input [ADDR_W-1:0]            ctr_addr,
    input [`DATA_W-1:0]           ctr_data_to_wr,

    //databus interface
    input                         databus_rnw,
    input [ADDR_W-1:0]            databus_addr,
    input [`DATA_W-1:0]           databus_data_in,
    input                         databus_mem_req,

    //input / output data
    input [2*`N*`DATA_W-1:0]      flow_in,
    output [2*`DATA_W-1:0]        flow_out,

    //configurations
    input [2*`MEMP_CONF_BITS-1:0] config_bits
    );

   //flow interface
   wire [`N*`DATA_W-1:0]          data_bus_prev = flow_in[2*`N*`DATA_W-1:`N*`DATA_W];
   
   wire [`DATA_W-1:0]             outA;
   wire [`DATA_W-1:0]             outB;
   
   assign flow_out = {outA, outB};



   
   function [`DADDR_W-1:0] reverseBits;
      input [`DADDR_W-1:0]        word;
      integer                     i;

      begin
	 for (i=0; i < `DADDR_W; i=i+1)
	   reverseBits[i]=word[`DADDR_W-1 - i];
      end
   endfunction

   wire wrA, wrB;

   //memory
`ifndef ASIC
   reg [`DATA_W-1:0] mem [2**ADDR_W-1:0];
`endif

   //port addresses and enables
   wire [`DADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [`DADDR_W-1:0] addrB, addrB_int, addrB_int2;

`ifdef XILINX
   wire 	       enA, enB;
`endif

   //data inputs
   wire [`DATA_W-1:0]  inA;
   wire [`DATA_W-1:0]  inB;

   //configuration
   wire [`PERIOD_W-1:0] perA;
   wire [`PERIOD_W-1:0] dutyA;
   wire [`DADDR_W-1:0]  iterationsA;

   wire [`N_W-1:0]      selA;
   wire [`DADDR_W-1:0]  startA;
   wire [`DADDR_W-1:0]  shiftA;
   wire [`DADDR_W-1:0]  incrA;
   wire [`PERIOD_W-1:0] delayA;
   wire 		reverseA;
   wire 		extA;

   wire [`PERIOD_W-1:0] perB;
   wire [`PERIOD_W-1:0] dutyB;
   wire [`DADDR_W-1:0]  iterationsB;

   wire [`N_W-1:0]      selB;
   wire [`DADDR_W-1:0]  startB;
   wire [`DADDR_W-1:0]  shiftB;
   wire [`DADDR_W-1:0]  incrB;
   wire [`PERIOD_W-1:0] delayB;
   wire 		reverseB;
   wire 		extB;

   //input enables
   wire 		in_enA, in_enB;

   //address gen enable
   wire                 enA_int, enB_int;

   //program ready
   wire                 readyA, readyB;

`ifdef ASIC
   wire 		enA, enB;
`endif

`ifdef XILINX
   reg [`DATA_W-1:0]    outA_int, outB_int;
`else
   wire [`DATA_W-1:0]   outA_int, outB_int;
`endif

   wire [`DATA_W-1:0]   data_to_wrA;
   wire [`DATA_W-1:0]   data_to_wrB;

   // ports program ready
   assign readyA = |iterationsA;
   assign readyB = |iterationsB;

   //unpack config data
   assign iterationsA = config_bits[2*`MEMP_CONF_BITS-1 -: `DADDR_W];
   assign perA = config_bits[2*`MEMP_CONF_BITS-`DADDR_W-1 -: `PERIOD_W];
   assign dutyA = config_bits[2*`MEMP_CONF_BITS-`DADDR_W-`PERIOD_W-1 -: `PERIOD_W];
   assign selA = config_bits[2*`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-1 -: `N_W];
   assign startA = config_bits[2*`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-1 -: `DADDR_W];
   assign shiftA = config_bits[2*`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-`DADDR_W-1 -: `DADDR_W];
   assign incrA = config_bits[2*`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-2*`DADDR_W-1 -: `DADDR_W];
   assign delayA = config_bits[2*`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-3*`DADDR_W-1 -: `PERIOD_W];
   assign reverseA = config_bits[2*`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-3*`DADDR_W-`PERIOD_W-1 -: 1];
   assign extA = config_bits[2*`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-3*`DADDR_W-`PERIOD_W-1-1 -: 1];

   assign iterationsB = config_bits[`MEMP_CONF_BITS-1 -: `DADDR_W];
   assign perB = config_bits[`MEMP_CONF_BITS-`DADDR_W-1 -: `PERIOD_W];
   assign dutyB = config_bits[`MEMP_CONF_BITS-`DADDR_W-`PERIOD_W-1 -: `PERIOD_W];
   assign selB = config_bits[`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-1 -: `N_W];
   assign startB = config_bits[`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-1 -: `DADDR_W];
   assign shiftB = config_bits[`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-`DADDR_W-1 -: `DADDR_W];
   assign incrB = config_bits[`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-2*`DADDR_W-1 -: `DADDR_W];
   assign delayB = config_bits[`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-3*`DADDR_W-1 -: `PERIOD_W];
   assign reverseB = config_bits[`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-3*`DADDR_W-`PERIOD_W-1 -: 1];
   assign extB = config_bits[`MEMP_CONF_BITS-`DADDR_W-2*`PERIOD_W-`N_W-3*`DADDR_W-`PERIOD_W-1-1 -: 1];


   //compute enables
`ifdef XILINX
   assign enA = 1'b1;
   assign enB = 1'b1;
`endif

`ifdef ASIC
   assign enA = enA_int|databus_mem_req;
   assign enB = enB_int|ctr_mem_req;
`endif

   assign wrA = databus_mem_req? ~databus_rnw : (enA_int & in_enA & ~extA); //addrgen on & input on & input isn't address
   assign wrB = ctr_mem_req? ~ctr_rnw : (enB_int & in_enB & ~extB);

   assign outA = (selA == `sADDR)?
		 {{(`DATA_W-`DADDR_W){addrA_int2[`DADDR_W-1]}}, addrA_int2} : outA_int;
   assign outB = (selB == `sADDR)?
		 {{(`DATA_W-`DADDR_W){addrB_int2[`DADDR_W-1]}}, addrB_int2}: outB_int;

   assign data_to_wrA = databus_mem_req? databus_data_in : inA ;
   assign data_to_wrB = ctr_mem_req?  ctr_data_to_wr : inB;


   //input selection
   xinmux muxa (
		.sel(selA),
		.data_bus(data_bus),
    .data_bus_prev(data_bus_prev),
		.data_out(inA),
		.enabled(in_enA)
		);

   xinmux  muxb (
		 .sel(selB),
		 .data_bus(data_bus),
     .data_bus_prev(data_bus_prev),
		 .data_out(inB),
		 .enabled(in_enB)
		 );

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

   assign addrA = databus_mem_req? databus_addr[ADDR_W-1:0] : extB? inB[ADDR_W-1:0] : addrA_int2[ADDR_W-1:0];
   assign addrB = ctr_mem_req? ctr_addr : extA? inA[ADDR_W-1:0] : addrB_int2[ADDR_W-1:0];

   assign addrA_int2 = reverseA? reverseBits(addrA_int) : addrA_int;
   assign addrB_int2 = reverseB? reverseBits(addrB_int) : addrB_int;

   //memory implementations
`ifdef XILINX

   //Initialize memory
   `ifdef INIT_MEM
       initial $readmemh(MEM_INIT_FILE, mem, 0, 2**ADDR_W - 1);
   `endif

   //PORT A
   always @ (posedge clk) begin
      if (enA) begin
	 if (wrA) begin
           mem[addrA] <= data_to_wrA;
	   outA_int <= data_to_wrA;
         end else
	   outA_int <= mem[addrA];
      end
   end

   //PORT B
   always @ (posedge clk) begin
    if (enB) begin
	    if (wrB) begin
              mem[addrB] <= data_to_wrB;
              outB_int <= data_to_wrB;
            end else
              outB_int <= mem[addrB];
      end
   end
`endif //  `ifdef XILINX

`ifdef ALTERA

   xalt_t2p_mem  #(
       .MEM_INIT_FILE(MEM_INIT_FILE),
		   .DATA_W(`DATA_W),
		   .ADDR_W(ADDR_W))
   alt_mem
     (
      .data_a(data_to_wrA),
      .data_b(data_to_wrB),
      .addr_a(addrA[ADDR_W-1:0]),
      .addr_b(addrB[ADDR_W-1:0]),
      .we_a(wrA),
      .we_b(wrB),
      .q_a(outA_int),
      .q_b(outB_int),
      .clk(clk)
      );


`endif //  `ifdef ALTERA

`ifdef ASIC

   xmem_wrapper2048x32 memory_wrapper (
				       .clk(clk),
				       //PORT A
				       .enA(enA),
				       .wrA(wrA),
				       .addrA(addrA),
				       .data_inA(data_to_wrA),
				       .data_outA(outA_int),

				       //PORT B
				       .enB(enB),
				       .wrB(wrB),
				       .addrB(addrB),
				       .data_inB(data_to_wrB),
				       .data_outB(outB_int)
				       );
`endif //  `ifdef ASIC

endmodule
