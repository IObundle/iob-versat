`timescale 1ns / 1ps

`define ALU_FNS_W       4
`define ALU_CONF_BITS (2*`N_W + `ALU_FNS_W)

// ALU functions
`define ALU_ADD           `ALU_FNS_W'd0
`define ALU_SUB           `ALU_FNS_W'd1
`define ALU_CMP_SIG       `ALU_FNS_W'd2    
`define ALU_MUX        `ALU_FNS_W'd3
`define ALU_MAX           `ALU_FNS_W'd4      
`define ALU_MIN           `ALU_FNS_W'd5
`define ALU_OR            `ALU_FNS_W'd6
`define ALU_AND           `ALU_FNS_W'd7
`define ALU_CMP_UNS       `ALU_FNS_W'd8
`define ALU_XOR           `ALU_FNS_W'd9
`define ALU_SEXT8         `ALU_FNS_W'd10
`define ALU_SEXT16        `ALU_FNS_W'd11
`define ALU_SHIFTR_ARTH   `ALU_FNS_W'd12
`define ALU_SHIFTR_LOG    `ALU_FNS_W'd13
`define ALU_CLZ           `ALU_FNS_W'd14
`define ALU_ABS           `ALU_FNS_W'd15

//ALU internal configuration address offsets
`define ALU_CONF_SELA     0
`define ALU_CONF_SELB     1
`define ALU_CONF_FNS      2
`define ALU_CONF_OFFSET   3

//ALU latency
`define ALU_LAT 2

`define ALULITE_FNS_W     4
`define ALULITE_CONF_BITS (`ALULITE_FNS_W)

// ALULITE functions
// concat a 1 to the left to get the feedback versions of these functions
`define ALULITE_ADD              3'd0
`define ALULITE_SUB              3'd1
`define ALULITE_CMP_SIG          3'd2    
`define ALULITE_MUX              3'd3
`define ALULITE_MAX              3'd4     
`define ALULITE_MIN              3'd5
`define ALULITE_OR               3'd6
`define ALULITE_AND              3'd7

//ALULITE internal configuration address offsets
`define ALULITE_CONF_SELA        0
`define ALULITE_CONF_SELB        1
`define ALULITE_CONF_FNS         2
`define ALULITE_CONF_OFFSET      3

//ALULITE latency
`define ALULITE_LAT               2
/*

 Description: simpler ALU with feedback

 */

module xalulite # ( 
       parameter          DATA_W = 32
   ) (
                 input  clk,
                 input  rst,
                 input  run,
                 input  running,

                 // Inputs
                 input  [DATA_W-1:0] in1,
                 input  [DATA_W-1:0] in2,

                 // Output
                 output reg [DATA_W-1:0] out,

                 // Config interface
                 input self_loop, 
                 input [`ALULITE_FNS_W-2 : 0] fns
                 );


   reg [DATA_W:0]                                 ai;
   reg [DATA_W:0]                                 bz;
   wire [DATA_W:0]                                temp_adder;
   reg                                            cin;

   reg signed [DATA_W-1:0]                        result_int;

   reg                                            in1_msb;
   reg                                            in2_msb;
   wire [DATA_W-1:0]                              in1_int;
   reg [DATA_W-1:0]                               in1_reg;
   reg [DATA_W-1:0]                               in2_reg;

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
