`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

module my_iob_dp_ram #(
   parameter FILE   = "none",
   parameter DATA_W = 8,
   parameter ADDR_W = 6
) (
   input clk_i,

   // Port A
   input      [DATA_W-1:0] dinA_i,
   input      [ADDR_W-1:0] addrA_i,
   input                   enA_i,
   input                   weA_i,
   output reg [DATA_W-1:0] doutA_o,

   // Port B
   input      [DATA_W-1:0] dinB_i,
   input      [ADDR_W-1:0] addrB_i,
   input                   enB_i,
   input                   weB_i,
   output reg [DATA_W-1:0] doutB_o
);

   //this allows ISE 14.7 to work; do not remove
   localparam mem_init_file_int = FILE;


   // Declare the RAM
   reg [DATA_W-1:0] ram[2**ADDR_W-1:0];

   // Initialize the RAM
   initial if (mem_init_file_int != "none") $readmemh(mem_init_file_int, ram, 0, 2 ** ADDR_W - 1);

   always @(posedge clk_i) begin  // Port A
      if (enA_i) begin
         if (weA_i) begin
            ram[addrA_i] <= dinA_i;
         end else begin
            doutA_o <= ram[addrA_i];
         end
      end
   end

   always @(posedge clk_i) begin  // Port B
      if (enB_i) begin
         if (weB_i) begin
            ram[addrB_i] <= dinB_i;
         end else begin
            doutB_o <= ram[addrB_i];
         end
      end
   end

endmodule  // my_iob_dp_ram

`default_nettype wire
