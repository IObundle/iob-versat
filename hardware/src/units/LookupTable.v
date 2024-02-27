`timescale 1ns / 1ps

module LookupTable #(
   parameter INIT_MEM_FILE = "none",
   parameter DATA_W        = 32,
   parameter SIZE_W        = 32,
   parameter ADDR_W        = 8
) (
   //databus interface
   input      [DATA_W/8-1:0] wstrb,
   input      [  ADDR_W-1:0] addr,
   input      [  DATA_W-1:0] wdata,
   input                     valid,
   output reg                rvalid,
   output     [  DATA_W-1:0] rdata,

   input [DATA_W-1:0] in0,
   input [DATA_W-1:0] in1,

   (* versat_latency = 3 *) output reg [DATA_W-1:0] out0,
   (* versat_latency = 3 *) output reg [DATA_W-1:0] out1,

   output [ADDR_W-1:0] ext_dp_addr_0_port_0,
   output [DATA_W-1:0] ext_dp_out_0_port_0,
   input  [DATA_W-1:0] ext_dp_in_0_port_0,
   output              ext_dp_enable_0_port_0,
   output              ext_dp_write_0_port_0,

   output [ADDR_W-1:0] ext_dp_addr_0_port_1,
   output [DATA_W-1:0] ext_dp_out_0_port_1,
   input  [DATA_W-1:0] ext_dp_in_0_port_1,
   output              ext_dp_enable_0_port_1,
   output              ext_dp_write_0_port_1,

   input running,
   input clk,
   input rst,
   input run
);

   assign rdata = 0;

   reg [DATA_W-1:0] data;
   reg              write;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         data     <= 0;
         write    <= 1'b0;
         rvalid   <= 1'b0;
      end else begin
         write  <= 1'b0;
         rvalid <= 1'b0;

         if (valid && wstrb == 0) begin
            rvalid <= 1'b1;
         end

         if (valid && |wstrb) begin
            data     <= wdata;
            write    <= 1'b1;
         end
      end
   end

   wire [DATA_W-1:0] outA, outB;

   assign ext_dp_out_0_port_0    = data;
   assign ext_dp_enable_0_port_0 = 1'b1;
   assign ext_dp_write_0_port_0  = write;

   assign ext_dp_out_0_port_1    = 0;
   assign ext_dp_enable_0_port_1 = 1'b1;
   assign ext_dp_write_0_port_1  = 1'b0;

   localparam DIFF = DATA_W / SIZE_W;
   localparam DECISION_BIT_W = $clog2(DIFF);

   reg [ADDR_W-1:0] addr_0_reg,addr_1_reg;

   always @(posedge clk,posedge rst) begin
      if(rst) begin
         addr_0_reg <= 0;
         addr_1_reg <= 0;
      end else begin
         if (valid && |wstrb) begin
            addr_0_reg <= addr;
         end else begin
            addr_0_reg <= in0[ADDR_W-1:0];
         end
         addr_1_reg <= in1[ADDR_W-1:0];;
      end
   end

   assign ext_dp_addr_0_port_0 = addr_0_reg;
   assign ext_dp_addr_0_port_1 = addr_1_reg;

   wire [DATA_W-1:0] outA_int,outB_int;

   generate
      if (SIZE_W == DATA_W) begin
         assign outA_int = ext_dp_in_0_port_0;
         assign outB_int = ext_dp_in_0_port_1;
      end else begin
         reg [DECISION_BIT_W-1:0] sel_A, sel_B;
         reg [DECISION_BIT_W-1:0] sel_A_reg, sel_B_reg;

         always @(posedge clk, posedge rst) begin
            if (rst) begin
               sel_A <= 0;
               sel_B <= 0;
            end else begin
               sel_A_reg <= in0[DECISION_BIT_W-1:0];
               sel_B_reg <= in1[DECISION_BIT_W-1:0];
               sel_A <= sel_A_reg;
               sel_B <= sel_B_reg;
            end
         end

         WideAdapter #(
            .INPUT_W (DATA_W),
            .SIZE_W  (SIZE_W),
            .OUTPUT_W(DATA_W)
         ) adapter_a (
            .sel_i(sel_A),
            .in_i (ext_dp_in_0_port_0),
            .out_o(outA_int)
         );

         WideAdapter #(
            .INPUT_W (DATA_W),
            .SIZE_W  (SIZE_W),
            .OUTPUT_W(DATA_W)
         ) adapter_b (
            .sel_i(sel_B),
            .in_i (ext_dp_in_0_port_1),
            .out_o(outB_int)
         );
      end
   endgenerate

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         out0 <= 0;
         out1 <= 0;
      end else begin
         out0 <= outA_int;
         out1 <= outB_int;
      end
   end

endmodule
