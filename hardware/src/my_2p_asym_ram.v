`timescale 1ns / 1ps

`define max(a, b) (a) > (b) ? (a) : (b)
`define min(a, b) (a) < (b) ? (a) : (b)

// Addresses are given in byte space
// The total size in bits is given by multiplying 2^ADDR_W * min(A_DATA_W,B_DATA_W)
// The number of blocks used is equal to max([A|B]_DATA_W) / min([A|B]_DATA_W)

module my_2p_asym_ram #(
   parameter W_DATA_W = 0,
   parameter R_DATA_W = 0,
   parameter ADDR_W   = 0
) (
   input                     clk_i,
   //write port
   input                     w_en_i,
   input      [W_DATA_W-1:0] w_data_i,
   input      [  ADDR_W-1:0] w_addr_i,
   //read port
   input                     r_en_i,
   input      [  ADDR_W-1:0] r_addr_i,
   output reg [R_DATA_W-1:0] r_data_o
);

   //determine the number of blocks N
   localparam MAXDATA_W =
   `max(W_DATA_W, R_DATA_W);
   localparam MINDATA_W =
   `min(W_DATA_W, R_DATA_W);
   localparam N = MAXDATA_W / MINDATA_W;
   localparam N_W = $clog2(N);
   localparam SYMBOL_W = $clog2(MINDATA_W / 8);

   //symmetric memory block buses
   //write buses
   reg  [        N-1:0] en_wr;
   reg  [MINDATA_W-1:0] data_wr[N-1:0];

   //read buses
   wire [MINDATA_W-1:0] data_rd[N-1:0];

   //instantiate N symmetric RAM blocks and connect them to the buses
   genvar i;
   generate
      for (i = 0; i < N; i = i + 1) begin
         iob_ram_2p #(
            .DATA_W(MINDATA_W),
            .ADDR_W(ADDR_W - (N_W + SYMBOL_W))
         ) iob_2p_ram_inst (
            .clk_i   (clk_i),
            .w_en_i  (en_wr[i]),
            .w_addr_i(w_addr_i[ADDR_W-1:N_W+SYMBOL_W]),
            .w_data_i(data_wr[i]),
            .r_en_i  (r_en_i),
            .r_addr_i(r_addr_i[ADDR_W-1:N_W+SYMBOL_W]),
            .r_data_o(data_rd[i])
         );

      end
   endgenerate

   integer j, k, l;
   generate

      if (W_DATA_W > R_DATA_W) begin

         //write parallel
         always @* begin
            for (j = 0; j < N; j = j + 1) begin
               en_wr[j]   = w_en_i;
               data_wr[j] = w_data_i[j*MINDATA_W+:MINDATA_W];
            end
         end

         //read address register
         reg [ADDR_W-1:0] r_addr_i_lsbs_reg;
         always @(posedge clk_i) r_addr_i_lsbs_reg <= r_addr_i[ADDR_W-1:0] >> SYMBOL_W;

         //read mux
         always @* begin
            r_data_o = 1'b0;
            for (l = 0; l < N; l = l + 1) begin
               r_data_o = data_rd[r_addr_i_lsbs_reg];
            end
         end

      end else if (W_DATA_W < R_DATA_W) begin
         wire [ADDR_W-1:0] byteAddr = w_addr_i[ADDR_W-1:0] >> SYMBOL_W;

         //write serial
         always @* begin
            for (j = 0; j < N; j = j + 1) begin
               en_wr[j]   = w_en_i & (byteAddr[N_W-1:0] == j[N_W-1:0]);
               data_wr[j] = w_data_i;
            end
         end
         //read parallel
         always @* begin
            r_data_o = 1'b0;
            for (k = 0; k < N; k = k + 1) begin
               r_data_o[k*MINDATA_W+:MINDATA_W] = data_rd[k];
            end
         end

      end else begin  //W_DATA_W = R_DATA_W
         //write serial
         always @* begin
            en_wr[0]   = w_en_i;
            data_wr[0] = w_data_i;
         end
         //read parallel
         always @(data_rd[0]) begin
            r_data_o = data_rd[0];
         end
      end
   endgenerate
endmodule  // my_2p_asym_ram

`undef max
`undef min

