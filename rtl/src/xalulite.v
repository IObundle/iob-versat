`timescale 1ns / 1ps
`include "xversat.vh"
`include "xalulitedefs.vh"

/*

 Description: simpler ALU with feedback

 */

module xalulite # ( 
		 parameter			  DATA_W = 32
	) (
                 input                            clk,
                 input                            rst,

                 //flow interface
                 input [2*`DATABUS_W-1:0]         flow_in,
                 output reg [DATA_W-1:0]    	  flow_out,

                 // config interface
                 input [`ALULITE_CONF_BITS - 1:0] configdata
                 );


   reg [DATA_W:0]                                 ai;
   reg [DATA_W:0]                                 bz;
   wire [DATA_W:0]                                temp_adder;
   wire [5:0]                                     data_out_clz_i;
   reg                                            cin;

   reg signed [DATA_W-1:0]                        result_int;

   wire [`N_W-1: 0]                               sela;
   wire [`N_W-1: 0]                               selb;

   wire [DATA_W-1:0]                              op_a;
   wire [DATA_W-1:0]                              op_b;
   reg                                            op_a_msb;
   reg                                            op_b_msb;
   wire [DATA_W-1:0]                              op_a_int;
   reg [DATA_W-1:0]                               op_a_reg;
   reg [DATA_W-1:0]                               op_b_reg;
   wire [`ALULITE_FNS_W-2:0]                      fns;
   wire                                           self_loop;

   // Unpack config data
   assign sela = configdata[`ALULITE_CONF_BITS-1 -: `N_W];
   assign selb = configdata[`ALULITE_CONF_BITS-`N_W-1 -: `N_W];
   assign self_loop = configdata[`ALULITE_FNS_W-1];
   assign fns = configdata[`ALULITE_FNS_W-2 : 0];

   // Input selection
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

   always @ (posedge clk, posedge rst)
     if (rst) begin
	op_b_reg <= {DATA_W{1'b0}};
	op_a_reg <= {DATA_W{1'b0}};
     end else begin
	op_b_reg <= op_b;
	op_a_reg <= op_a;
     end

   assign op_a_int = self_loop? flow_out : op_a_reg;

   // Computes result_int
   always @ * begin

      result_int = temp_adder[31:0];

      case (fns)
	`ALULITE_OR : begin
	   result_int = op_a_int | op_b_reg;
	end
	`ALULITE_AND : begin
	   result_int = op_a_int & op_b_reg;
	end
	`ALULITE_CMP_SIG : begin
	   result_int[31] = temp_adder[32] ;
	end
	`ALULITE_MUX : begin
	   result_int = op_b_reg;

	   if(~op_a_reg[31]) begin
	      if(self_loop)
	        result_int = flow_out;
	      else
	        result_int = {DATA_W{1'b0}};
	   end
	end
	`ALULITE_SUB : begin
	end
	`ALULITE_ADD : begin
	   if(self_loop) begin
	      if(op_a_reg[31])
	        result_int = op_b_reg;
	   end
	end
	`ALULITE_MAX : begin
	   if (temp_adder[32] == 1'b0) begin
	      result_int = op_b_reg;
           end else begin
	      result_int = op_a_int;
           end

	   if(self_loop) begin
	      if(op_a_reg[31])
	        result_int = flow_out;
	   end
	end
	`ALULITE_MIN : begin
	   if (temp_adder[32] == 1'b0) begin
	      result_int = op_a_int;
           end else begin
	      result_int = op_b_reg;
           end

	   if(self_loop) begin
	      if(op_a_reg[31])
	        result_int = flow_out;
	   end
	end
	default : begin
	end
      endcase // case (fns)
   end

   // Computes temp_adder
   assign temp_adder = ((bz & ({op_b_msb,op_b_reg}))) + ((ai ^ ({op_a_msb,op_a_int}))) + {{32{1'b0}},cin};

   // Compute ai, cin, bz
   always @ * begin
      cin = 1'b 0;
      ai = {33{1'b0}}; // will invert op_a_int if set to all ones
      bz = {33{1'b1}}; // will zero op_b_reg if set to all zeros

      op_a_msb = 1'b0;
      op_b_msb = 1'b0;

      case(fns)
	`ALULITE_CMP_SIG : begin
	   ai = {33{1'b1}};
	   cin = 1'b 1;
	   op_a_msb = op_a_reg[31];
	   op_b_msb = op_b_reg[31];
	end
	`ALULITE_SUB : begin
	   ai = {33{1'b1}};
	   cin = 1'b 1;
	end
	`ALULITE_MAX : begin
           ai = {33{1'b1}};
           cin = 1'b 1;
	   op_a_msb = op_a_int[31];
	   op_b_msb = op_b_reg[31];
	end
	`ALULITE_MIN : begin
           ai = {33{1'b1}};
           cin = 1'b 1;
	   op_a_msb = op_a_int[31];
	   op_b_msb = op_b_reg[31];
	end
	default : begin
	end
      endcase

   end

   always @ (posedge clk, posedge rst)
     if (rst)
       flow_out <= {DATA_W{1'b0}};
     else 
       flow_out <= result_int;

endmodule
