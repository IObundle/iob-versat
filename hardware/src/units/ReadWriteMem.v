`timescale 1ns / 1ps

`define COMPLEX_INTERFACE

// Probably could be implemented using Mem.v but for now simplify
(* source *) module ReadWriteMem #(
   parameter MEM_INIT_FILE = "none",
   parameter DATA_W        = 32,
   parameter SIZE_W        = 32,
   parameter ADDR_W        = 12
) (
   //control
   input clk,
   input rst,

   input  running,
   input  run,
   output done,
   input  disabled,

   //databus interface
   input      [DATA_W/8-1:0] wstrb,
   input      [  ADDR_W-1:0] addr,
   input      [  DATA_W-1:0] wdata,
   input                     valid,
   output reg                rvalid,
   output     [  DATA_W-1:0] rdata,

   //input / output data
                            input  [DATA_W-1:0] in0,
   (* versat_latency = 3 *) output [DATA_W-1:0] out0,

   // External memory
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

   input [    32-1:0] delay0,

   // Configuration
   // Input config
   input [ADDR_W-1:0] iterA,
   input [       9:0] perA,
   input [       9:0] dutyA,
   input [ADDR_W-1:0] startA,
   input [ADDR_W-1:0] shiftA,
   input [ADDR_W-1:0] incrA,
   input              reverseA,
   input              extA,
   input              in0_wr,

   // Output config
   input [ADDR_W-1:0] iterB,
   input [       9:0] perB,
   input [       9:0] dutyB,
   input [ADDR_W-1:0] startB,
   input [ADDR_W-1:0] shiftB,
   input [ADDR_W-1:0] incrB,
   //input [    32-1:0] delay1,
   input              reverseB
);

   wire we = |wstrb;

   wire doneA, doneB;

   //output databus
   reg [DATA_W-1:0] out_reg;

   assign out0 = (running ? out_reg : 0);
   assign done = (doneA & doneB);

   //function to reverse bits
   function [ADDR_W-1:0] reverseBits;
      input [ADDR_W-1:0] word;
      integer i;

      begin
         for (i = 0; i < ADDR_W; i = i + 1) reverseBits[i] = word[ADDR_W-1-i];
      end
   endfunction

   //mem enables output by addr gen
   wire enA_int;
   wire enA = enA_int | valid;
   wire enB;

   //write enables
   wire wrA = valid ? we : (enA_int & in0_wr & ~extA); //addrgen on & input on & input isn't address

   //port addresses and enables
   wire [ADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   //data inputs
   wire [DATA_W-1:0] data_to_wrA = valid ? wdata : in0;

   //address generators

   MyAddressGen #(
      .ADDR_W(ADDR_W),
      .DATA_W(SIZE_W)
   ) addrgenA (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run && !disabled),

      //configurations 
      .period_i(perA),
      .start_i (startA),
      .incr_i  (incrA),
      .delay_i (delay0),

`ifdef COMPLEX_INTERFACE
      .iterations_i(iterA),
      .duty_i      (dutyA),
      .shift_i     (shiftA),
`endif

      //outputs 
      .valid_o(enA_int),
      .ready_i(1'b1),
      .addr_o (addrA_int),
      .done_o (doneA)
   );

   MyAddressGen #(
      .ADDR_W(ADDR_W),
      .DATA_W(SIZE_W)
   ) addrgenB (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run && !disabled),

      //configurations 
      .period_i(perB),
      .start_i (startB),
      .incr_i  (incrB),
      .delay_i (0),

`ifdef COMPLEX_INTERFACE
      .iterations_i(iterB),
      .duty_i      (dutyB),
      .shift_i     (shiftB),
`endif

      //outputs 
      .valid_o(enB),
      .ready_i(1'b1),
      .addr_o (addrB_int),
      .done_o (doneB)
   );

   //define addresses based on ext and rvrs
   assign addrA      = valid ? addr[ADDR_W-1:0] : (extA ? in0[ADDR_W-1:0] : addrA_int2[ADDR_W-1:0]);
   assign addrB      = valid ? addr[ADDR_W-1:0] : addrB_int2[ADDR_W-1:0];
   assign addrA_int2 = reverseA ? reverseBits(addrA_int) : addrA_int;
   assign addrB_int2 = reverseB ? reverseBits(addrB_int) : addrB_int;

   //register mem inputs
   reg [DATA_W-1:0] data_a_reg;
   reg [ADDR_W-1:0] addr_a_reg, addr_b_reg;
   reg en_a_reg, en_b_reg, we_a_reg;
   always @(posedge clk) begin
      data_a_reg <= data_to_wrA;
      addr_a_reg <= addrA;
      addr_b_reg <= addrB;
      en_a_reg   <= enA;
      en_b_reg   <= (valid && !we);
      we_a_reg   <= wrA;
   end

   // Read 
   assign rdata = (rvalid ? out_reg : 32'h0);

   // Delay by a cycle to match memory read latency
   reg readCounter[1:0];
   always @(posedge clk, posedge rst) begin
      if (rst) begin
         rvalid         <= 1'b0;
         readCounter[0] <= 1'b0;
         readCounter[1] <= 1'b0;
      end else begin
         if (rvalid) begin
            rvalid         <= 1'b0;
            readCounter[0] <= 1'b0;
            readCounter[1] <= 1'b0;
         end else begin
            // Write
            //if(valid && we)
            //   ready <= 1'b1;

            // Read
            rvalid         <= readCounter[0];
            readCounter[0] <= readCounter[1];
            if (wstrb == 0) readCounter[1] <= valid;
         end
      end
   end

   assign ext_dp_addr_0_port_0   = addr_a_reg;
   assign ext_dp_out_0_port_0    = data_a_reg;
   assign ext_dp_enable_0_port_0 = en_a_reg;
   assign ext_dp_write_0_port_0  = we_a_reg;

   assign ext_dp_addr_0_port_1   = addr_b_reg;
   assign ext_dp_out_0_port_1    = 0;
   assign ext_dp_enable_0_port_1 = en_b_reg || running;
   assign ext_dp_write_0_port_1  = 1'b0;

   //register mem outputs
   always @(posedge clk, posedge rst) begin
      if (rst) begin
         out_reg <= 0;
      end else if (run) begin
         out_reg <= 0;
      end else begin
         out_reg <= ext_dp_in_0_port_1;
      end
   end

endmodule

`undef COMPLEX_INTERFACE
