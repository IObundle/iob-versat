`timescale 1ns / 1ps

`define max(a, b) (a) > (b) ? (a) : (b)
`define min(a, b) (a) < (b) ? (a) : (b)

// Addresses are given in byte space. 
// The total size in bits is given by multiplying 2^ADDR_W * min(A_DATA_W,B_DATA_W)
// The number of blocks used is equal to max([A|B]_DATA_W) / min([A|B]_DATA_W)

module my_dp_asym_ram #(
   parameter FILE     = "none",
   parameter A_DATA_W = 8,
   parameter B_DATA_W = 8,
   parameter ADDR_W   = 6
) (
   input clk_i,

   // Port A
   input      [A_DATA_W-1:0] dinA_i,
   input      [  ADDR_W-1:0] addrA_i,
   input                     enA_i,
   input                     weA_i,
   output reg [A_DATA_W-1:0] doutA_o,

   // Port B
   input      [B_DATA_W-1:0] dinB_i,
   input      [  ADDR_W-1:0] addrB_i,
   input                     enB_i,
   input                     weB_i,
   output reg [B_DATA_W-1:0] doutB_o
);

   //determine the number of blocks N
   localparam MAXDATA_W =
   `max(A_DATA_W, B_DATA_W);
   localparam MINDATA_W =
   `min(A_DATA_W, B_DATA_W);
   localparam N = MAXDATA_W / MINDATA_W;
   localparam N_W = $clog2(N);
   localparam SYMBOL_W = $clog2(MINDATA_W / 8);

   localparam M_ADDR_W = ADDR_W - N_W;

   //symmetric memory block buses
   //write buses
   reg  [        N-1:0] bus_enA_i;
   reg  [        N-1:0] bus_weA_i;
   reg  [MINDATA_W-1:0] bus_inA   [N-1:0];
   wire [MINDATA_W-1:0] bus_outA  [N-1:0];

   reg  [        N-1:0] bus_enB_i;
   reg  [        N-1:0] bus_weB_i;
   reg  [MINDATA_W-1:0] bus_inB   [N-1:0];
   wire [MINDATA_W-1:0] bus_outB  [N-1:0];

   //instantiate N symmetric RAM blocks and connect them to the buses
   genvar i;
   generate
      for (i = 0; i < N; i = i + 1) begin  : ramInst
         my_iob_dp_ram #(
            .DATA_W(MINDATA_W),
            .ADDR_W(M_ADDR_W - SYMBOL_W)
         ) iob_dp_ram_inst (
            .clk_i(clk_i),

            // Port A
            .dinA_i (bus_inA[i]),
            .addrA_i(addrA_i[ADDR_W-1:SYMBOL_W+N_W]),
            .enA_i  (bus_enA_i[i]),
            .weA_i  (bus_weA_i[i]),
            .doutA_o(bus_outA[i]),

            // Port B
            .dinB_i (bus_inB[i]),
            .addrB_i(addrB_i[ADDR_W-1:SYMBOL_W+N_W]),
            .enB_i  (bus_enB_i[i]),
            .weB_i  (bus_weB_i[i]),
            .doutB_o(bus_outB[i])
         );
      end
   endgenerate

   reg [N-1:0] selector;

   generate
      if (A_DATA_W > B_DATA_W) begin
         always @(posedge clk_i) if (enB_i) selector <= addrB_i[N_W+SYMBOL_W-1:SYMBOL_W];
      end else if (A_DATA_W < B_DATA_W) begin
         always @(posedge clk_i) if (enA_i) selector <= addrA_i[N_W+SYMBOL_W-1:SYMBOL_W];
      end
   endgenerate

   integer j, k, l;
   generate
      if (A_DATA_W > B_DATA_W) begin
         // Port A
         always @* begin
            for (j = 0; j < N; j = j + 1) begin
               bus_enA_i[j]                    = enA_i;
               bus_weA_i[j]                    = weA_i;
               bus_inA[j]                      = dinA_i[j*MINDATA_W+:MINDATA_W];
               doutA_o[j*MINDATA_W+:MINDATA_W] = bus_outA[j];
            end
         end

         // Port B         
         always @* begin
            for (k = 0; k < N; k = k + 1) begin
               if (selector == k) begin
                  doutB_o = bus_outB[k];
               end

               if (addrB_i[N_W+SYMBOL_W-1:SYMBOL_W] == k) begin
                  bus_enB_i[k] = enB_i;
               end else begin
                  bus_enB_i[k] = 1'b0;
               end
               bus_weB_i[k] = weB_i;
               bus_inB[k]   = dinB_i;
            end
         end

      end else if (A_DATA_W < B_DATA_W) begin
         // Port B
         always @* begin
            for (j = 0; j < N; j = j + 1) begin
               bus_enB_i[j]                    = enB_i;
               bus_weB_i[j]                    = weB_i;
               bus_inB[j]                      = dinB_i[j*MINDATA_W+:MINDATA_W];
               doutB_o[j*MINDATA_W+:MINDATA_W] = bus_outB[j];
            end
         end

         // Port A
         always @* begin
            for (k = 0; k < N; k = k + 1) begin
               if (selector == k) begin
                  doutA_o = bus_outA[k];
               end

               if (addrA_i[N_W+SYMBOL_W-1:SYMBOL_W] == k) begin
                  bus_enA_i[k] = enA_i;
               end else begin
                  bus_enA_i[k] = 1'b0;
               end
               bus_weA_i[k] = weA_i;
               bus_inA[k]   = dinA_i;
            end
         end
      end else begin  //A_DATA_W = B_DATA_W
         // Port A
         always @* begin
            bus_enA_i[0] = enA_i;
            bus_weA_i[0] = weA_i;
            bus_inA[0]   = dinA_i;
            doutA_o      = bus_outA[0];
         end

         // Port B
         always @* begin
            bus_enB_i[0] = enB_i;
            bus_weB_i[0] = weB_i;
            bus_inB[0]   = dinB_i;
            doutB_o      = bus_outB[0];
         end
      end
   endgenerate

endmodule  // my_dp_asym_ram

`undef max
`undef min
