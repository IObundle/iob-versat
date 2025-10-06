`timescale 1ns / 1ps

// Buffer for simple handshake signals. 
// Think of a skid buffer but without registering the valid/ready signals, meaning that we keep the combinatorial path over the entire pipeline
// Should have the same interface as a skid buffer, meaning that a simple change in modules should still work and improve performance, if needed

// What this unit accomplishes is that we basically have a "pull" interface, since this unit is always asserting in_ready_o when it can.
// Helpful to break ready/valid dependencies, since this unit does not need to wait for in_valid_i to become asserted before asserting ready_o

module HandShakeBuffer #(
   parameter DATA_W = 0
)(
   input  in_valid_i,
   output reg in_ready_o,
   input [DATA_W-1:0] in_data_i,

   output reg out_valid_o,
   input out_ready_i,
   output [DATA_W-1:0] out_data_o,

   input clk_i,
   input rst_i
);

reg data_valid;
reg [DATA_W-1:0] data_stored;

assign out_data_o = data_stored;
assign out_valid_o = data_valid;

always @(posedge clk_i,posedge rst_i) begin
   if(rst_i) begin
      data_valid <= 1'b0;
      data_stored <= 0;
   end else begin
      // If no data, can always transfer
      if(!data_valid && in_valid_i && in_ready_o) begin
         data_valid <= 1'b1;
         data_stored <= in_data_i;
      end

      // If data, can always output, and we then have no data
      if(data_valid && out_valid_o && out_ready_i) begin
         data_valid <= 1'b0;
      end

      // Unless the input is also trying to transfer, at which point we still have valid data
      if(in_ready_o && in_valid_i && out_valid_o && out_ready_i) begin
         data_valid <= 1'b1;
         data_stored <= in_data_i;
      end
   end
end

always @* begin
   in_ready_o = 1'b0;

    // If no data, can always transfer
   if(!data_valid)
      in_ready_o = 1'b1;

   // Data is moved out and moved in at the same time.
   // This is the combinatorial path that a skid buffer would break
   // It also puts a dependency on in_valid/in_ready
   if(in_valid_i && out_valid_o && out_ready_i)
      in_ready_o = 1'b1;

end

endmodule
