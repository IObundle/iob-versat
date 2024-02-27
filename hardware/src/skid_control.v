`timescale 1ns / 1ps

module skid_control (
   input      in_valid_i,
   output reg in_ready_o,
   output     in_transfer_o,

   output reg out_valid_o,
   input      out_ready_i,
   output     out_transfer_o,

   input      valid_i, // Assert if data is valid_i (something like a unit that takes a couple of cycles to process data would assert this indicate valid_iity)
   input enable_transfer_i,  // If unit allows transfer to occur

   output reg store_data_o,
   output reg use_stored_data_o,

   input clk_i,
   input rst_i
);

   assign in_transfer_o  = (in_valid_i && in_ready_o);
   assign out_transfer_o = (out_valid_o && out_ready_i && enable_transfer_i);

   reg has_stored;

   always @(posedge clk_i, posedge rst_i) begin
      if (rst_i) begin
         has_stored <= 1'b0;
         in_ready_o <= 1'b1;
      end else begin
         case ({
            !enable_transfer_i, has_stored, in_valid_i, out_ready_i
         })
            4'b0000: begin
               in_ready_o <= 1'b1;
            end
            4'b0001: begin
               in_ready_o <= 1'b1;
            end
            4'b0010: begin
               has_stored <= 1'b1;
               in_ready_o <= 1'b0;
            end
            4'b0011: begin
               in_ready_o <= 1'b1;
            end
            4'b0100: begin
               in_ready_o <= 1'b0;
            end
            4'b0101: begin
               has_stored <= 1'b0;
               in_ready_o <= 1'b1;
            end
            4'b0110: begin
               in_ready_o <= 1'b0;
            end
            4'b0111: begin
               in_ready_o <= 1'b1;
            end
            4'b1000: begin
               in_ready_o <= 1'b1;
            end
            4'b1001: begin
               in_ready_o <= 1'b1;
            end
            4'b1010: begin
               has_stored <= 1'b1;
               in_ready_o <= 1'b0;
            end
            4'b1011: begin
               has_stored <= 1'b1;
               in_ready_o <= 1'b0;
            end
            4'b1100: begin
               in_ready_o <= 1'b0;
            end
            4'b1101: begin
               in_ready_o <= 1'b0;
            end
            4'b1110: begin
               in_ready_o <= 1'b0;
            end
            4'b1111: begin
               in_ready_o <= 1'b0;
            end
         endcase
      end
   end

   always @* begin
      out_valid_o       = 1'b0;
      store_data_o      = 1'b0;
      use_stored_data_o = has_stored;

      case ({
         !enable_transfer_i, has_stored, in_valid_i, out_ready_i
      })
         4'b0000: begin
         end
         4'b0001: begin
         end
         4'b0010: begin
            out_valid_o = valid_i;
         end
         4'b0011: begin
            out_valid_o = valid_i;
         end
         4'b0100: begin
            out_valid_o = valid_i;
         end
         4'b0101: begin
            out_valid_o = valid_i;
         end
         4'b0110: begin
            out_valid_o = valid_i;
         end
         4'b0111: begin
            out_valid_o = valid_i;
         end
         4'b1000: begin
         end
         4'b1001: begin
         end
         4'b1010: begin
            out_valid_o = valid_i;
         end
         4'b1011: begin
            out_valid_o = valid_i;
         end
         4'b1100: begin
            out_valid_o = valid_i;
         end
         4'b1101: begin
            out_valid_o = valid_i;
         end
         4'b1110: begin
            out_valid_o = valid_i;
         end
         4'b1111: begin
            out_valid_o = valid_i;
         end
      endcase
   end

endmodule  // skid_control
