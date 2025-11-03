`timescale 1ns / 1ps

// Only intented for simulation. Address in byteSpace
module SimHelper_DatabusMem #(
   parameter ADDR_W = 1,
   parameter DATA_W = 32,
   parameter LEN_W  = 8
) (
   output                      databus_ready,
   input                       databus_valid,
   input      [    ADDR_W-1:0] databus_addr,
   output reg [    DATA_W-1:0] databus_rdata,
   input      [    DATA_W-1:0] databus_wdata,
   input      [(DATA_W/8)-1:0] databus_wstrb,
   input      [     LEN_W-1:0] databus_len,
   output                      databus_last,

   input              insertValue,
   input [ADDR_W-1:0] addrToInsert,
   input [DATA_W-1:0] valueToInsert,

   input clk,
   input rst
);

   localparam OFFSET = $clog2(DATA_W / 8);
   localparam MEMSIZE = 2 ** (ADDR_W - OFFSET);
   reg [DATA_W-1:0] mem        [MEMSIZE-1:0];

   reg [       1:0] state;
   reg [ LEN_W-1:0] counter;
   reg [ADDR_W-1:0] addr;
   reg              read_valid;

   assign databus_ready = ((state == 2'h1 && read_valid) || state == 2'h2);
   assign databus_last  = (counter == 0 && (state != 2'h0));

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         state      <= 0;
         counter    <= 0;
         addr       <= 0;
         read_valid <= 0;
      end else begin
         if (insertValue) begin
            mem[addrToInsert>>OFFSET] <= valueToInsert;
         end

         case (state)
            2'h0:
            if (databus_valid) begin
               counter <= {{OFFSET{1'b0}}, databus_len[(LEN_W-1):OFFSET]};
               addr    <= databus_addr;

               if (|databus_wstrb == 0) begin
                  state <= 2'h1;
               end else begin
                  state <= 2'h2;
               end
            end
            2'h1: begin  // Read
               if (databus_valid) begin
                  read_valid    <= 1'b1;
                  databus_rdata <= mem[addr>>OFFSET];
                  counter       <= counter - 1;
                  addr          <= addr + (1 << OFFSET);
                  if (counter == 0) begin
                     state <= 2'h0;
                  end
               end
            end
            2'h2: begin
               if (databus_valid) begin
                  counter           <= counter - 1;
                  mem[addr>>OFFSET] <= databus_wdata;
                  addr              <= addr + (1 << OFFSET);
                  if (counter == 0) begin
                     state <= 2'h0;
                  end
               end
            end
         endcase
      end
   end

endmodule
