`timescale 1ns / 1ps

`define COMPLEX_INTERFACE

module OnlyOutputMem #(
   parameter DATA_W        = 32,
   parameter SIZE_W        = 32,
   parameter DELAY_W       = 7,
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
   (* versat_latency = 2 *) output [DATA_W-1:0] out0,

   // External memory
   output [    ADDR_W-1:0] ext_2p_addr_out_0,
   output [    ADDR_W-1:0] ext_2p_addr_in_0,
   output                  ext_2p_write_0,
   output                  ext_2p_read_0,
   input  [DATA_W-1:0] ext_2p_data_in_0,
   output [DATA_W-1:0] ext_2p_data_out_0,

   // Configuration
   input [ADDR_W-1:0] iterA,
   input [       9:0] perA,
   input [       9:0] dutyA,
   input [ADDR_W-1:0] startA,
   input [ADDR_W-1:0] shiftA,
   input [ADDR_W-1:0] incrA,
   input [DELAY_W-1:0] delay0,
   input              reverseA,
   input              extA,
   input [ADDR_W-1:0] iter2A,
   input [       9:0] per2A,
   input [ADDR_W-1:0] shift2A,
   input [ADDR_W-1:0] incr2A
);

   wire we = |wstrb;

   wire doneA;

   //output databus
   wire [DATA_W-1:0] outA;
   reg [DATA_W-1:0] outA_reg;

   assign out0 = running ? outA_reg : 0;

   assign done = doneA;

   //port addresses and enables
   wire [ADDR_W-1:0] addrA;
   wire enA;

   //address generators

   AddressGen #(
      .ADDR_W(ADDR_W),
      .DATA_W(SIZE_W),
      .DELAY_W(DELAY_W)
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
      .valid_o(enA),
      .ready_i(1'b1),
      .addr_o (addrA),
      .store_o(), // TODO: Handle duty

      .done_o (doneA)
   );
   
   //register mem inputs
   reg [DATA_W-1:0] data_a_reg;
   reg [ADDR_W-1:0] addr_a_reg;
   reg we_a_reg;
   always @(posedge clk,posedge rst) begin
      if(rst) begin
         data_a_reg <= 0;
         addr_a_reg <= 0;
         we_a_reg   <= 0;
      end else begin
         data_a_reg <= wdata;
         addr_a_reg <= addr;
         we_a_reg   <= valid & we;
      end
   end

   assign ext_2p_addr_out_0 = addr_a_reg;
   assign ext_2p_write_0    = we_a_reg;
   assign ext_2p_data_out_0 = data_a_reg;

   assign ext_2p_addr_in_0 = addrA;
   assign ext_2p_read_0    = enA;
   
   //register mem outputs
   always @(posedge clk, posedge rst) begin
      if (rst) begin
         outA_reg <= 0;
      end else begin
         outA_reg <= ext_2p_data_in_0;  //outA;
      end
   end

endmodule

`undef COMPLEX_INTERFACE
