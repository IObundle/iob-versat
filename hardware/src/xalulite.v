`timescale 1ns / 1ps
`include "xversat.vh"
`include "xdefs.vh"
`include "xalulitedefs.vh"

/*

 Description: simpler ALU with feedback

 */

module xalulite # ( 
       parameter          DATA_W = 32
   ) (
                 input  clk,
                 input  rst,

                 // Inputs
                 input  [DATA_W-1:0] in1,
                 input  [DATA_W-1:0] in2,

                 // Output
                 output reg [DATA_W-1:0] out,

                 // Config interface
                 input [`ALULITE_CONF_BITS - 1:0] configdata
                 );


   reg [DATA_W:0]                                 ai;
   reg [DATA_W:0]                                 bz;
   wire [DATA_W:0]                                temp_adder;
   reg                                            cin;

   reg signed [DATA_W-1:0]                        result_int;

   wire [DATA_W-1:0]                              in1;
   wire [DATA_W-1:0]                              in2;
   reg                                            in1_msb;
   reg                                            in2_msb;
   wire [DATA_W-1:0]                              in1_int;
   reg [DATA_W-1:0]                               in1_reg;
   reg [DATA_W-1:0]                               in2_reg;
   wire [`ALULITE_FNS_W-2:0]                      fns;
   wire                                           self_loop;

   // Unpack config data
   assign self_loop = configdata[`ALULITE_FNS_W-1];
   assign fns = configdata[`ALULITE_FNS_W-2 : 0];

   always @ (posedge clk, posedge rst)
     if (rst) begin
   in2_reg <= {DATA_W{1'b0}};
   in1_reg <= {DATA_W{1'b0}};
     end else begin
   in2_reg <= in2;
   in1_reg <= in1;
     end

   assign in1_int = self_loop? out : in1_reg;

   // Computes result_int
   always @ * begin

      result_int = temp_adder[DATA_W-1:0];

      case (fns)
   `ALULITE_OR : begin
      result_int = in1_int | in2_reg;
   end
   `ALULITE_AND : begin
      result_int = in1_int & in2_reg;
   end
   `ALULITE_CMP_SIG : begin
      result_int[DATA_W-1] = temp_adder[DATA_W] ;
   end
   `ALULITE_MUX : begin
      result_int = in2_reg;

      if(~in1_reg[DATA_W-1]) begin
         if(self_loop)
           result_int = out;
         else
           result_int = {DATA_W{1'b0}};
      end
   end
   `ALULITE_SUB : begin
   end
   `ALULITE_ADD : begin
      if(self_loop) begin
         if(in1_reg[DATA_W-1])
           result_int = in2_reg;
      end
   end
   `ALULITE_MAX : begin
      if (temp_adder[DATA_W] == 1'b0) begin
         result_int = in2_reg;
           end else begin
         result_int = in1_int;
           end

      if(self_loop) begin
         if(in1_reg[DATA_W-1])
           result_int = out;
      end
   end
   `ALULITE_MIN : begin
      if (temp_adder[DATA_W] == 1'b0) begin
         result_int = in1_int;
           end else begin
         result_int = in2_reg;
           end

      if(self_loop) begin
         if(in1_reg[DATA_W-1])
           result_int = out;
      end
   end
   default : begin
   end
      endcase // case (fns)
   end

   // Computes temp_adder
   assign temp_adder = ((bz & ({in2_msb,in2_reg}))) + ((ai ^ ({in1_msb,in1_int}))) + {{DATA_W{1'b0}},cin};

   // Compute ai, cin, bz
   always @ * begin
      cin = 1'b 0;
      ai = {DATA_W+1{1'b0}}; // will invert in1_int if set to all ones
      bz = {DATA_W+1{1'b1}}; // will zero in2_reg if set to all zeros

      in1_msb = 1'b0;
      in2_msb = 1'b0;

      case(fns)
   `ALULITE_CMP_SIG : begin
      ai = {DATA_W+1{1'b1}};
      cin = 1'b 1;
      in1_msb = in1_reg[DATA_W-1];
      in2_msb = in2_reg[DATA_W-1];
   end
   `ALULITE_SUB : begin
      ai = {DATA_W+1{1'b1}};
      cin = 1'b 1;
   end
   `ALULITE_MAX : begin
           ai = {DATA_W+1{1'b1}};
           cin = 1'b 1;
      in1_msb = in1_int[DATA_W-1];
      in2_msb = in2_reg[DATA_W-1];
   end
   `ALULITE_MIN : begin
           ai = {DATA_W+1{1'b1}};
           cin = 1'b 1;
      in1_msb = in1_int[DATA_W-1];
      in2_msb = in2_reg[DATA_W-1];
   end
   default : begin
   end
      endcase

   end

   always @ (posedge clk, posedge rst)
     if (rst)
       out <= {DATA_W{1'b0}};
     else 
       out <= result_int;

endmodule
