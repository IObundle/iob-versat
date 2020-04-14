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
                input [2*`N*`DATA_W-1:0]      flow_in,
                output [2*`DATA_W-1:0]        flow_out,

                // config interface
                input [`MULADD_CONF_BITS-1:0] configdata
                );

   //flow interface
   wire [`N*`DATA_W-1:0]          data_bus_prev = flow_in[2*`N*`DATA_W-1:`N*`DATA_W];
   
   reg signed [`DATA_W-1:0]       result;
   
   assign flow_out = result;

   reg                                        rst_reg;

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

   // enabled operands and result
   wire                                       enablea;
   wire                                       enableb;
   wire                                       enableo;
   wire                                       enabled;

   // register muladd control
   reg [`DATA_W-1:0]                          op_o_reg;

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
   assign opcode = configdata[`MULADD_FNS_W-1: 0];

   //input selection
   xinmux muxa (
                .sel(sela),
                .data_bus(data_bus),
                .data_bus_prev(data_bus_prev),
                .data_out(op_a),
		.enabled(enablea)
 		);

   xinmux muxb (
                .sel(selb),
                .data_bus(data_bus),
                .data_bus_prev(data_bus_prev),
                .data_out(op_b),
		.enabled(enableb)
		);

   xinmux muxo (
                .sel(selo),
                .data_bus(data_bus),
                .data_bus_prev(data_bus_prev),
                .data_out(op_o),
		.enabled(enableo)
		);

   // compute output enable
   assign enabled = enablea & enableb & enableo;

   always @ (posedge clk)
     rst_reg <= rst;

   //update registers
   always @ (posedge clk)
     if (rst_reg) begin
        acc <= {2*`DATA_W{1'b0}};
     op_o_reg <= `DATA_W'd0;
`ifndef MULADD_COMB                             //pipelined
     ld_acc1 <= 1'b0;
     ld_acc2 <= 1'b0;
`endif
  end else if (enabled) begin
     acc <= result64;
     op_o_reg <= op_o;
`ifndef MULADD_COMB                             //pipelined
     ld_acc1 <= ld_acc0;
     ld_acc2 <= ld_acc1;
`endif
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
		        .rst(rst_reg),
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

   assign result = result64[2*`DATA_W-1 : `DATA_W];

endmodule
