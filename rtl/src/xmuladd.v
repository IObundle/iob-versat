`timescale 1ns/1ps
`include "xversat.vh"
`include "xmuladddefs.vh"

/*

 Description: multiply (accumulate) functions

 */

module xmuladd (
                input                         rst,
                input                         clk,
                //flow interface
                input [2*`DATABUS_W-1:0]      flow_in,
                output [`DATA_W-1:0] 	      flow_out,

                // config interface
                input [`MULADD_CONF_BITS-1:0] configdata
                );

   wire [`MULADD_FNS_W:0]                     opcode;
   wire signed [2*`DATA_W-1:0]                result_mult;
   reg [2*`DATA_W-1:0]                        result64;
   reg [2*`DATA_W-1:0]                        acc;

   //data
   wire signed [`DATA_W-1:0]                  op_a;
   wire signed [`DATA_W-1:0]                  op_b;
   wire [`DATA_W-1:0]                         op_o;

   //config data
   wire [`N_W-1: 0]                           sela;
   wire [`N_W-1: 0]                           selb;
   wire [`N_W-1: 0]                           selo;
   wire [`PERIOD_W-1:0]                       delay;

   // enabled operands and result
   wire                                       enablea;
   wire                                       enableb;
   wire                                       enableo;
   wire                                       enabled;

   // register muladd control
   reg [`DATA_W-1:0]                          op_o_reg;
   reg [`PERIOD_W-1:0]			      rst_cnt;

   // accumulator load signal
   wire                                       ld_acc;
   //combinatorial
   wire                                       ld_acc0;
   //pipelined
`ifndef MULADD_COMB
   reg                                        ld_acc1;
   reg                                        ld_acc2;
`endif

   //unpack config bits
   assign sela   = configdata[`MULADD_CONF_BITS-1 -: `N_W];
   assign selb   = configdata[`MULADD_CONF_BITS-1-`N_W -: `N_W];
   assign selo   = configdata[`MULADD_CONF_BITS-1-2*`N_W -: `N_W];
   assign opcode = configdata[`MULADD_CONF_BITS-1-3*`N_W -: `MULADD_FNS_W];
   assign delay  = configdata[`PERIOD_W-1: 0] + `MULADD_LAT;

   //input selection
   xinmux muxa (
                .sel(sela),
                .data_in(flow_in),
                .data_out(op_a)
 		);

   xinmux muxb (
                .sel(selb),
                .data_in(flow_in),
                .data_out(op_b)
		);

   xinmux muxo (
                .sel(selo),
                .data_in(flow_in),
                .data_out(op_o)
		);

   //update registers
   always @ (posedge clk, posedge rst) begin
     if (rst) begin
       acc <= {2*`DATA_W{1'b0}};
       op_o_reg <= `DATA_W'd0;
       rst_cnt <= `PERIOD_W'd0;
`ifndef MULADD_COMB                             //pipelined
       ld_acc1 <= 1'b0;
       ld_acc2 <= 1'b0;
`endif
     end else begin
       if(rst_cnt < delay) 
         rst_cnt += 1'd1; 
       else begin
         acc <= result64;
         op_o_reg <= op_o;
`ifndef MULADD_COMB                             //pipelined
         ld_acc1 <= ld_acc0;
         ld_acc2 <= ld_acc1;
`endif
       end
     end
   end

   assign ld_acc0 = (op_o_reg==`DATA_W'd0);

   // compute accumulator load signal
`ifdef MULADD_COMB                             //combinatorial
   assign ld_acc = ld_acc0;
`else                                          //pipelined
   assign ld_acc = ld_acc2;
`endif

   // select multiplier statically
`ifdef MULADD_COMB                             //combinatorial
   assign result_mult = op_a * op_b;
`else                                          //2-stage pipeline
   xmul_pipe xmul_pipe (
		        .rst(rst),
		        .clk(clk),
		        .op_a(op_a),
		        .op_b(op_b),
		        .product(result_mult)
		        );
`endif

   // process mult result according to opcode
   always @ * begin

      case (opcode)
	`MULADD_MUL_DIV2:
	  result64 = result_mult&64'hFFFFFFFF00000000;
	`MULADD_MUL:
	  result64 = result_mult << 1;
	`MULADD_MACC_DIV2: begin
           if(ld_acc)
             result64 =  result_mult&64'hFFFFFFFF00000000;
           else
	     result64 =  acc + result_mult&64'hFFFFFFFF00000000;
	end
	`MULADD_MSUB_DIV2: begin
           if(ld_acc)
             result64 =  result_mult;
           else
	     result64 =  acc - result_mult;
 	end
	`MULADD_MACC: begin
           if(ld_acc)
             result64 =  (result_mult<<1)&64'hFFFFFFFF00000000;
           else
	     result64 =  acc + (result_mult<<1)&64'hFFFFFFFF00000000;
 	end
	`MULADD_MACC_16Q16: begin
           if(ld_acc) begin
              result64 =  {result_mult[47:16], 32'b0};
           end
           else begin
              result64 =  acc + {result_mult[47:16], 32'b0};
           end
 	end
        
	`MULADD_MSUB: begin
           if(ld_acc)
             result64 =  result_mult<<1;
           else
	     result64 =  acc - (result_mult<<1);
        end
	`MULADD_MUL_LOW:
	  result64 =  result_mult<<32;

	`MULADD_MUL_LOW_MACC: begin
	   result64 = acc + (result_mult << 32);
	end

	default: //MACC
          result64 = acc + (result_mult<<1);
      endcase // case (opcode)
   end

   assign flow_out = result64[2*`DATA_W-1 : `DATA_W];

endmodule
