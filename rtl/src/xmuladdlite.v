`timescale 1ns/1ps
`include "xversat.vh"
`include "xmuladdlitedefs.vh"

module xmuladdlite # (
		parameter		      DATA_W = 32
	) (
                input                         rst,
                input                         clk,
                input			      addrgen_rst,

                //flow interface
                input [2*`DATABUS_W-1:0]      flow_in,
                output [DATA_W-1:0] 	      flow_out,

                // config interface
                input [`MULADDLITE_CONF_BITS-1:0] configdata
                );

   //double precision data
   reg signed [2*DATA_W-1:0]		      result_mult;
   reg [2*DATA_W-1:0]                         out_reg;
   wire [2*DATA_W-1:0]                        shifted, bias;

   //data
   wire signed [DATA_W-1:0]                   op_a, op_b, op_c;
   wire [`MEM_ADDR_W-1:0]                     op_o;
   reg signed [DATA_W-1:0]                    op_a_r0, op_b_r0, op_c_r0, op_c_r1;
   wire [DATA_W-1:0]                          result;
   reg [DATA_W-1:0]                           result_reg;

   //config data
   wire [`N_W-1:0]                            sela, selb, selc;
   wire [`MEM_ADDR_W-1:0]		      iterations;
   wire [`PERIOD_W-1:0]                       period, delay;
   wire [`SHIFT_W-1:0]			      shift;
   wire                                       accIN, accOUT, batch;

   //accumulator load signal
   wire                                       ld_acc;
   reg                                        ld_acc0, ld_acc1, ld_acc2, ld_acc3, ld_acc4;

   //unpack config bits
   assign sela = configdata[`MULADDLITE_CONF_BITS-1 -: `N_W];
   assign selb = configdata[`MULADDLITE_CONF_BITS-1-`N_W -: `N_W];
   assign selc = configdata[`MULADDLITE_CONF_BITS-1-2*`N_W -: `N_W];
   assign iterations = configdata[`MULADDLITE_CONF_BITS-1-3*`N_W -: `MEM_ADDR_W];
   assign period = configdata[`MULADDLITE_CONF_BITS-1-3*`N_W-`MEM_ADDR_W -: `PERIOD_W];
   assign delay = configdata[`MULADDLITE_CONF_BITS-1-3*`N_W-`MEM_ADDR_W-`PERIOD_W -: `PERIOD_W];
   assign shift = configdata[`MULADDLITE_CONF_BITS-1-3*`N_W-`MEM_ADDR_W-2*`PERIOD_W -: `SHIFT_W];
   assign accIN = configdata[`MULADDLITE_CONF_BITS-1-3*`N_W-`MEM_ADDR_W-2*`PERIOD_W-`SHIFT_W -: 1];
   assign accOUT = configdata[`MULADDLITE_CONF_BITS-1-3*`N_W-`MEM_ADDR_W-2*`PERIOD_W-`SHIFT_W-1 -: 1];
   assign batch = configdata[`MULADDLITE_CONF_BITS-1-3*`N_W-`MEM_ADDR_W-2*`PERIOD_W-`SHIFT_W-2 -: 1];

   //input selection
   xinmux # (
	.DATA_W(DATA_W)
   ) muxa (
        .sel(sela),
        .data_in(flow_in),
        .data_out(op_a)
 	);

   xinmux # (
	.DATA_W(DATA_W)
   ) muxb (
        .sel(selb),
        .data_in(flow_in),
        .data_out(op_b)
	);

   xinmux # (
	.DATA_W(DATA_W)
   ) muxc (
        .sel(selc),
        .data_in(flow_in),
        .data_out(op_c)
	);

   //addr_gen to control macc
   wire ready = |iterations;
   wire mem_en, done;
   xaddrgen addrgen (
	.clk(clk),
	.rst(addrgen_rst),
	.init(rst),
	.run(rst & ready),
	.pause(1'b0),
	.iterations(iterations),
	.period(period),
	.duty(period),
	.start(`MEM_ADDR_W'b0),
	.shift(-period),
	.incr(`MEM_ADDR_W'b1),
	.delay(delay),
	.addr(op_o),
	.mem_en(mem_en),
	.done(done)
	);

   //update registers
   always @ (posedge clk, posedge rst) begin
     if (rst) begin
       ld_acc0 <= 1'b0;
       ld_acc1 <= 1'b0;
       ld_acc2 <= 1'b0;
       ld_acc3 <= 1'b0;
       ld_acc4 <= 1'b0;
       out_reg <= {2*DATA_W{1'b0}};
       op_a_r0 <= {DATA_W{1'b0}};
       op_b_r0 <= {DATA_W{1'b0}};
       op_c_r0 <= {DATA_W{1'b0}};
       op_c_r1 <= {DATA_W{1'b0}};
       result_reg <= {DATA_W{1'b0}};
     end else begin
       ld_acc0 <= ld_acc;
       ld_acc1 <= ld_acc0;
       ld_acc2 <= ld_acc1;
       ld_acc3 <= ld_acc2;
       ld_acc4 <= ld_acc3;
       out_reg <= shifted;
       op_a_r0 <= op_a;
       op_b_r0 <= op_b;
       op_c_r0 <= op_c;
       op_c_r1 <= op_c_r0;
       result_reg <= result;
     end
   end

   //concatenate
   assign bias = (ld_acc || !accIN) ? {{DATA_W{1'b0}},op_c_r0} : {op_c_r0,op_c_r1};

   //compute accumulator load signal
   assign ld_acc = (op_o=={`MEM_ADDR_W{1'd0}});

   //multiplier signals and regs
   reg signed [DATA_W-1:0] op_a_reg, op_b_reg;
   reg signed [2*DATA_W-1:0] acc, dsp_out, op_c_reg;
   wire signed [2*DATA_W-1:0] acc_w, bias_w;

   //4-stage multiplier (DSP48E2 template)
   always @ (posedge clk) begin
     op_a_reg <= op_a_r0;
     op_b_reg <= op_b_r0;
     result_mult <= op_a_reg * op_b_reg;
     op_c_reg <= bias_w;
     acc <= acc_w + result_mult;
     dsp_out <= acc;
   end
   assign acc_w = ld_acc2 ? op_c_reg : acc;

   //select accumulation initial value
   assign bias_w = batch ? bias << shift : accIN ? bias : {2*DATA_W{1'b0}};

   //output 1st or 2nd half
   assign shifted = dsp_out >> shift;
   assign result = (ld_acc4 & accOUT) ? out_reg[2*DATA_W-1 -: DATA_W] : shifted[DATA_W-1 : 0];
   assign flow_out = result_reg;

endmodule
