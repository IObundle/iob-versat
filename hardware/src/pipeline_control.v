`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

// Usually for a unit that can always process the data in one cycle and needs to be integrated into a handshake chain
module pipeline_control
  (
   input      in_valid_i,
   output reg in_ready_o,
   output     in_transfer_o,

   output reg out_valid_o,
   input      out_ready_i,
   output     out_transfer_o,

   input      clk_i,
   input      rst_i  
   );

assign in_transfer_o = (in_valid_i && in_ready_o);
assign out_transfer_o = (out_valid_o && out_ready_i);

always @*
begin
  in_ready_o = 1'b1; // Always ready
  out_valid_o = 1'b0;

  if(in_ready_o && in_valid_i)
    out_valid_o = 1'b1;
end

endmodule // pipeline_control

`default_nettype wire
