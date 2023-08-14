`timescale 1ns/1ps

// Usually for a unit that can always process the data in one cycle and needs to be integrated into a handshake chain
`default_nettype none
module pipeline_control(
    input in_valid,
    output reg in_ready,
    output in_transfer,

    output reg out_valid,
    input out_ready,
    output out_transfer,

    input clk,
    input rst  
  );

assign in_transfer = (in_valid && in_ready);
assign out_transfer = (out_valid && out_ready);

always @*
begin
  in_ready = 1'b1; // Always ready
  out_valid = 1'b0;

  if(in_ready && in_valid)
    out_valid = 1'b1;
end

endmodule
