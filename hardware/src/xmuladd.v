`timescale 1ns/1ps
`include "xversat.vh"
`include "xmuladddefs.vh"

/*

 Description: multiply (accumulate) functions

 */

module xmuladd # ( 
		parameter		      DATA_W = 32
	) (
                input                         rst,
                input                         clk,
                
                //input			      addrgen_rst,
                
                input [DATA_W-1:0]           in1,
                input [DATA_W-1:0]           in2,
                output [DATA_W-1:0]          out,

                // config interface
                input [`MULADD_CONF_BITS-1:0] configdata
                );

   wire addrgen_rst = configdata[2];

   wire [`MULADD_FNS_W-1:0]                   opcode;
   wire signed [2*DATA_W-1:0]                 result_mult;
   reg signed [2*DATA_W-1:0]		      result_mult_reg;
   reg [2*DATA_W-1:0]                         result;

   //data
   wire [`MEM_ADDR_W-1:0]                     op_o;

   //config data
   wire [`N_W-1: 0]                           sela;
   wire [`N_W-1: 0]                           selb;
   wire [`MEM_ADDR_W-1:0]		      iterations;
   wire [`PERIOD_W-1:0]                       period;
   wire [`PERIOD_W-1:0]                       delay;
   wire [`SHIFT_W-1:0]			      shift;

   // register muladd control
   reg [`MEM_ADDR_W-1:0]                      op_o_reg;

   // accumulator load signal
   wire                                       ld_acc;
   //combinatorial
   wire                                       ld_acc0;
   //pipelined
`ifdef MULADD_COMB
   reg [2*DATA_W-1:0]                         acc;
`else
   reg                                        ld_acc1;
`endif

   //unpack config bits
   assign sela   = configdata[`MULADD_CONF_BITS-1 -: `N_W];
   assign selb   = configdata[`MULADD_CONF_BITS-1-`N_W -: `N_W];
   assign opcode = configdata[`MULADD_CONF_BITS-1-2*`N_W -: `MULADD_FNS_W];
   assign iterations = configdata[`MULADD_CONF_BITS-1-2*`N_W-`MULADD_FNS_W -: `MEM_ADDR_W];
   assign period = configdata[`MULADD_CONF_BITS-1-2*`N_W-`MULADD_FNS_W-`MEM_ADDR_W -: `PERIOD_W];
   assign delay = configdata[`MULADD_CONF_BITS-1-2*`N_W-`MULADD_FNS_W-`MEM_ADDR_W-`PERIOD_W -: `PERIOD_W];
   assign shift = configdata[`MULADD_CONF_BITS-1-2*`N_W-`MULADD_FNS_W-`MEM_ADDR_W-2*`PERIOD_W -: `SHIFT_W];

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
       op_o_reg <= {`MEM_ADDR_W{1'd0}};
`ifdef MULADD_COMB                             //pipelined
       acc <= {2*DATA_W{1'b0}};
`else
       ld_acc1 <= 1'b0;
`endif
     end else begin
       op_o_reg <= op_o;
`ifdef MULADD_COMB                             //pipelined
       acc <= result;
`else
       ld_acc1 <= ld_acc0;
`endif
     end
   end

   assign ld_acc0 = (op_o_reg=={`MEM_ADDR_W{1'd0}});

   // compute accumulator load signal
`ifdef MULADD_COMB                             //combinatorial
   assign ld_acc = ld_acc0;
`else                                          //pipelined
   assign ld_acc = ld_acc1;
`endif

   // select multiplier statically
`ifdef MULADD_COMB                             //combinatorial
   always @ (posedge clk, posedge rst)
     if(rst)
       result_mult_reg = {2*DATA_W{1'b0}};
     else
       result_mult_reg = in1 * in2;
   assign result_mult = result_mult_reg;
`else                                          //3-stage pipeline
   reg signed [DATA_W-1:0] in1_reg, in2_reg;
   reg signed [2*DATA_W-1:0] acc, dsp_out;
   wire signed [2*DATA_W-1:0] acc_w;
   //DSP48E template
   always @ (posedge clk) begin
     in1_reg <= in1;
     in2_reg <= in2;
     result_mult_reg <= in1_reg * in2_reg;
     if(opcode == `MULADD_MACC)
       acc <= acc_w + result_mult_reg;
     else
       acc <= acc_w - result_mult_reg;
     dsp_out <= acc;
   end
   assign result = dsp_out;
   assign acc_w = ld_acc ? {DATA_W*2{1'b0}} : acc;
`endif

   // process mult result according to opcode
`ifdef MULADD_COMB
   always @ * begin

      case (opcode)
	`MULADD_MACC: begin
           if(ld_acc)
             result = result_mult;
           else
	     result = acc + result_mult;
 	end
	`MULADD_MSUB: begin
           if(ld_acc)
             result = result_mult;
           else
	     result = acc - result_mult;
        end
	default: //MACC
          result = acc + result_mult;
      endcase // case (opcode)
   end
`endif

   assign out = result >> shift;

endmodule
