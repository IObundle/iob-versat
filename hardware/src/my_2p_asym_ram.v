`timescale 1ns / 1ps
`define max(a,b) {(a) > (b) ? (a) : (b)}
`define min(a,b) {(a) < (b) ? (a) : (b)}

// Addresses are given in byte space
// The total size in bits is given by multiplying 2^ADDR_W * min(A_DATA_W,B_DATA_W)
// The number of blocks used is equal to max([A|B]_DATA_W) / min([A|B]_DATA_W)

module my_2p_asym_ram
  #(
    parameter W_DATA_W = 0,
    parameter R_DATA_W = 0,
    parameter ADDR_W = 0
    )
   (
    input                     clk,
    //write port
    input                     w_en,
    input [W_DATA_W-1:0]      w_data,
    input [ADDR_W-1:0]        w_addr,
    //read port
    input                     r_en,
    input [ADDR_W-1:0]        r_addr,
    output reg [R_DATA_W-1:0] r_data
    );

   //determine the number of blocks N
   localparam MAXDATA_W = `max(W_DATA_W, R_DATA_W);
   localparam MINDATA_W = `min(W_DATA_W, R_DATA_W);
   localparam N = MAXDATA_W/MINDATA_W;
   localparam N_W = $clog2(N);
   localparam SYMBOL_W = $clog2(MINDATA_W/8);

   //symmetric memory block buses
   //write buses
   reg [N-1:0]                en_wr;
   reg [MINDATA_W-1:0]        data_wr [N-1:0];
   reg [ADDR_W-1:0]        addr_wr [N-1:0];

   //read buses
   wire [MINDATA_W-1:0]       data_rd [N-1:0];
   reg [ADDR_W-1:0]        addr_rd [N-1:0];

   //instantiate N symmetric RAM blocks and connect them to the buses
   genvar                 i;
   generate 
      for (i=0; i<N; i=i+1) begin
         iob_2p_ram
             #(
               .DATA_W(MINDATA_W),
               .ADDR_W(ADDR_W)
               )
         iob_2p_ram_inst
             (
              .clk(clk),
              .w_en(en_wr[i]),
              .w_addr(addr_wr[i]),
              .w_data(data_wr[i]),
              .r_en(r_en),
              .r_addr(addr_rd[i]),
              .r_data(data_rd[i])              
              );
         
      end
   endgenerate

   integer j,k,l;
   generate

      if (W_DATA_W > R_DATA_W) begin
         
         //write parallel
         always @* begin
            for (j=0; j < N; j= j+1) begin
               en_wr[j] = w_en;
               data_wr[j] = w_data[j*MINDATA_W +: MINDATA_W];
               addr_wr[j] = w_addr >> (N_W + SYMBOL_W);
            end
         end
         
         //read serial
         always @* begin
            for (k=0; k < N; k= k+1) begin
               addr_rd[k] = r_addr[ADDR_W-1-:ADDR_W] >> SYMBOL_W;
            end
         end

         //read address register
         reg [ADDR_W-1:0] r_addr_lsbs_reg;
         always @(posedge clk)
           r_addr_lsbs_reg <= r_addr[ADDR_W-1:0] >> SYMBOL_W;
           
         //read mux
         always @* begin
            r_data = 1'b0;
            for (l=0; l < N; l= l+1) begin
               r_data = data_rd[r_addr_lsbs_reg];
            end
         end
         
      end else  if (W_DATA_W < R_DATA_W) begin
         wire [ADDR_W-1:0] byteAddr = w_addr[ADDR_W-1:0] >> SYMBOL_W;

         //write serial
         always @* begin
            for (j=0; j < N; j= j+1) begin
               en_wr[j] = w_en & (byteAddr[N_W-1:0] == j[N_W-1:0]);
               data_wr[j] = w_data;
               addr_wr[j] = w_addr >> (N_W + SYMBOL_W);
            end
         end
         //read parallel
         always @* begin
            r_data = 1'b0;
            for (k=0; k < N; k= k+1) begin
               addr_rd[k] = r_addr >> (N_W + SYMBOL_W);
               r_data[k*MINDATA_W +: MINDATA_W] = data_rd[k];
            end
         end

      end else begin //W_DATA_W = R_DATA_W
         //write serial
         always @* begin
            en_wr[0] = w_en;
            data_wr[0] = w_data;
            addr_wr[0] = w_addr >> SYMBOL_W;
         end
         //read parallel
         always @* begin
            addr_rd[0] = r_addr >> SYMBOL_W;
            r_data = data_rd[0];
         end
      end
   endgenerate
endmodule
