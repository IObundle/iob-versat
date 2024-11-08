`timescale 1ns / 1ps

// Buffer for simple handshake signals that breaks ready/valid signals dependencies while keeping a throughput of 1
// This means that multiple skid buffers in a row do not suffer from a large combinatorial path that spans the entire pipeline
// This is accomplished by having a second buffer that stores the extra data generated 
// from taking one extra cycle to properly react to the ready/signal values
// This and handshake buffer are equivalent interface wise, but this unit uses more memory and logic in order to achieve greater throughput

module SkidBuffer #(
   parameter DATA_W = 32
)(
   input  in_valid_i,
   output in_ready_o,
   input [DATA_W-1:0] in_data_i,

   output out_valid_o,
   input out_ready_i,
   output [DATA_W-1:0] out_data_o,

   input clk_i,
   input rst_i
);

localparam EMPTY = 2'b00,
           DATA = 2'b01,
           EXTRA = 2'b10;

reg [1:0] state;
reg [DATA_W-1:0] data_stored,extra_data_stored;

wire in_transfer = (in_valid_i & in_ready_o);
wire out_transfer = (out_valid_o & out_ready_i);

// Technically combinatorial but of a register on our side. Compiler could be smart enough to detect this, but probably
// need to add more info so that the compiler figures out the correct code.
assign in_ready_o = (state == EMPTY || state == DATA);
assign out_valid_o = (state == DATA || state == EXTRA);
assign out_data_o = data_stored;

always @(posedge clk_i,posedge rst_i) begin
   if(rst_i) begin
      state <= EMPTY;
      data_stored <= 0;
      extra_data_stored <= 0;
   end else begin
      case(state)
      EMPTY: begin // out_transfer cannot occur here
         if(in_transfer) begin
            state <= DATA;
            data_stored <= in_data_i;
         end
      end
      DATA: begin
         case({in_transfer,out_transfer})
         2'b00: begin
            // Nothing
         end
         2'b01: begin
            state <= EMPTY;
         end
         2'b10: begin
            state <= EXTRA;
            extra_data_stored <= in_data_i;
         end
         2'b11: begin
            data_stored <= in_data_i;
         end
         endcase
      end
      EXTRA: begin // in_transfer cannot occur here.
         if(out_transfer) begin
            state <= DATA;
            data_stored <= extra_data_stored;
         end
      end
      2'b11:; // Nothing
      endcase
   end
end

endmodule
