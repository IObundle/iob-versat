`timescale 1ns/1ps

`define max(a,b) {(a) > (b) ? (a) : (b)}
`define min(a,b) {(a) < (b) ? (a) : (b)}

// Addresses are given in byte space. 
// The total size in bits is given by multiplying 2^ADDR_W * min(A_DATA_W,B_DATA_W)
// The number of blocks used is equal to max([A|B]_DATA_W) / min([A|B]_DATA_W)

module my_dp_asym_ram
  #(
    parameter FILE = "none",
    parameter A_DATA_W = 8,
    parameter B_DATA_W = 8,
    parameter ADDR_W = 6
    )
   (
    input                   clk,

    // Port A
    input [A_DATA_W-1:0]      dinA,
    input [ADDR_W-1:0]        addrA,
    input                     enA,
    input                     weA,
    output reg [A_DATA_W-1:0] doutA,

    // Port B
    input [B_DATA_W-1:0]      dinB,
    input [ADDR_W-1:0]        addrB,
    input                     enB,
    input                     weB,
    output reg [B_DATA_W-1:0] doutB
    );

   //determine the number of blocks N
   localparam MAXDATA_W = `max(A_DATA_W, B_DATA_W);
   localparam MINDATA_W = `min(A_DATA_W, B_DATA_W);
   localparam N = MAXDATA_W/MINDATA_W;
   localparam N_W = $clog2(N);
   localparam SYMBOL_W = $clog2(MINDATA_W/8);

   localparam M_ADDR_W = ADDR_W - N_W;

   //symmetric memory block buses
   //write buses
   reg [N-1:0]             bus_enA;
   reg [N-1:0]             bus_weA;
   reg [M_ADDR_W-1:0]      bus_addrA[N-1:0];
   reg [MINDATA_W-1:0]     bus_inA[N-1:0];
   wire [MINDATA_W-1:0]    bus_outA[N-1:0];

   reg [N-1:0]             bus_enB;
   reg [N-1:0]             bus_weB;
   reg [M_ADDR_W-1:0]      bus_addrB[N-1:0];
   reg [MINDATA_W-1:0]     bus_inB[N-1:0];
   wire [MINDATA_W-1:0]    bus_outB[N-1:0];

   //instantiate N symmetric RAM blocks and connect them to the buses
   genvar                 i;
   generate 
      for (i=0; i<N; i=i+1) begin
         my_iob_dp_ram
             #(
               .DATA_W(MINDATA_W),
               .ADDR_W(M_ADDR_W)
               )
         iob_dp_ram_inst
             (
            .clk(clk),

            // Port A
            .dinA(bus_inA[i]),
            .addrA(bus_addrA[i]),
            .enA(bus_enA[i]),
            .weA(bus_weA[i]),
            .doutA(bus_outA[i]),

            // Port B
            .dinB(bus_inB[i]),
            .addrB(bus_addrB[i]),
            .enB(bus_enB[i]),
            .weB(bus_weB[i]),
            .doutB(bus_outB[i])
            );
      end
  endgenerate

   reg [N-1:0] selector;
   
   generate
      if (A_DATA_W > B_DATA_W) begin
         always @(posedge clk) if(enB) selector <= addrB[N_W+SYMBOL_W-1:SYMBOL_W];
      end else if (A_DATA_W < B_DATA_W) begin
         always @(posedge clk) if(enA) selector <= addrA[N_W+SYMBOL_W-1:SYMBOL_W];
      end
   endgenerate

   integer j,k,l;
   generate
      if (A_DATA_W > B_DATA_W) begin
         // Port A
         always @* begin
            for (j=0; j < N; j= j+1) begin
              bus_enA[j] = enA;
              bus_weA[j] = weA;
              bus_addrA[j] = addrA[ADDR_W-1:SYMBOL_W] >> N_W;
              bus_inA[j] = dinA[j*MINDATA_W +: MINDATA_W];
              doutA[j*MINDATA_W +: MINDATA_W] = bus_outA[j];
            end
         end

         // Port B         
         always @* begin
            for (k=0; k < N; k= k+1) begin
              if(selector == k) begin
               doutB = bus_outB[k];
              end

              if(addrB[N_W+SYMBOL_W-1:SYMBOL_W] == k) begin
                bus_enB[k] = enB;
              end else begin
                bus_enB[k] = 1'b0;
              end
              bus_weB[k] = weB;
              bus_addrB[k] = addrB[ADDR_W-1:SYMBOL_W] >> N_W;
              bus_inB[k] = dinB;
            end
         end
         
      end else  if (A_DATA_W < B_DATA_W) begin
         // Port B
         always @* begin
            for (j=0; j < N; j= j+1) begin
              bus_enB[j] = enB;
              bus_weB[j] = weB;
              bus_addrB[j] = addrB[ADDR_W-1:SYMBOL_W] >> N_W;
              bus_inB[j] = dinB[j*MINDATA_W +: MINDATA_W];
              doutB[j*MINDATA_W +: MINDATA_W] = bus_outB[j];
            end
         end

         // Port A
         always @* begin
            for (k=0; k < N; k= k+1) begin
              if(selector == k) begin
               doutA = bus_outA[k];
              end

              if(addrA[N_W+SYMBOL_W-1:SYMBOL_W] == k) begin
                bus_enA[k] = enA;
              end else begin
                bus_enA[k] = 1'b0;
              end
              bus_weA[k] = weA;
              bus_addrA[k] = addrA[ADDR_W-1:SYMBOL_W] >> N_W;
              bus_inA[k] = dinA;
            end
         end
      end else begin //A_DATA_W = B_DATA_W
         // Port A
         always @* begin
           bus_enA[0] = enA;
           bus_weA[0] = weA;
           bus_addrA[0] = addrA[ADDR_W-1:SYMBOL_W]; // Byte space to symbol space 
           bus_inA[0] = dinA;
           doutA = bus_outA[0];
         end
         
         // Port B
         always @* begin
           bus_enB[0] = enB;
           bus_weB[0] = weB;
           bus_addrB[0] = addrB[ADDR_W-1:SYMBOL_W]; // Byte space to symbol space
           bus_inB[0] = dinB;
           doutB = bus_outB[0];
         end         
      end
   endgenerate

endmodule   
