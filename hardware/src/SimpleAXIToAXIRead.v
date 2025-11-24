`timescale 1ns / 1ps

module SimpleAXIToAXIRead #(
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter AXI_LEN_W  = 8,
   parameter AXI_ID_W   = 4,
   parameter LEN_W      = 8
) (
   input                       m_rvalid_i,
   output reg                  m_rready_o,
   input      [AXI_ADDR_W-1:0] m_raddr_i,
   output     [AXI_DATA_W-1:0] m_rdata_o,
   input      [     LEN_W-1:0] m_rlen_i,
   output                      m_rlast_o,

   /*
      NOTE: Using the burst aligner to calculate the logic for rlast is error prone.
      At the beginning, we receive a length. Because the outside expects data aligned, it also means that it expects a constant amount of transfers.
      It does not matter if everything is align or not, the ouside code expects the same number of transfers for the same length.
      It is therefore easier to calculate the number of transfers from the length and to control the transfer logic from that aspect.
   */ 

   output [AXI_ID_W-1:0] axi_arid_o,
   output [AXI_ADDR_W-1:0] axi_araddr_o,
   output [AXI_LEN_W-1:0] axi_arlen_o,
   output [3-1:0] axi_arsize_o,
   output [2-1:0] axi_arburst_o,
   output [2-1:0] axi_arlock_o,
   output [4-1:0] axi_arcache_o,
   output [3-1:0] axi_arprot_o,
   output [4-1:0] axi_arqos_o,
   output [1-1:0] axi_arvalid_o,
   input [1-1:0] axi_arready_i,
   input [AXI_ID_W-1:0] axi_rid_i,
   input [AXI_DATA_W-1:0] axi_rdata_i,
   input [2-1:0] axi_rresp_i,
   input [1-1:0] axi_rlast_i,
   input [1-1:0] axi_rvalid_i,
   output [1-1:0] axi_rready_o,

   input clk_i,
   input rst_i
);

   localparam OFFSET_W = $clog2(AXI_DATA_W / 8);

   localparam [2:0] axi_size = ((AXI_DATA_W == 16)   ? 3'b001 : 
                                (AXI_DATA_W == 32)   ? 3'b010 :
                                (AXI_DATA_W == 64)   ? 3'b011 :
                                (AXI_DATA_W == 128)  ? 3'b100 :
                                (AXI_DATA_W == 256)  ? 3'b101 :
                                (AXI_DATA_W == 512)  ? 3'b110 : 
                                (AXI_DATA_W == 1024) ? 3'b111 : 3'b000);

   // Read

   assign axi_arid_o    = 0;
   assign axi_arsize_o  = axi_size;
   assign axi_arburst_o = 'b01;  // INCR
   assign axi_arlock_o  = 0;
   assign axi_arcache_o = 0;
   assign axi_arprot_o  = 0;
   assign axi_arqos_o   = 0;

   reg [1:0] read_state;
   reg arvalid, rready;

   assign axi_arvalid_o = arvalid;
   assign axi_rready_o  = (read_state == 2'h3);

   wire read_last_transfer;
   wire burst_i_align_empty;

   // Read
   BurstAlign #(
      .AXI_DATA_W(AXI_DATA_W)
   ) aligner (
      .offset_i(m_raddr_i[OFFSET_W-1:0]),
      .start_i (read_state == 0),

      .burst_last_i   (axi_rvalid_i && axi_rready_o && axi_rlast_i),
      .transfer_last_i(read_last_transfer),

      .last_transfer_o(/* m_rlast_o */),
      .empty_o        (burst_i_align_empty),

      // Simple interface for data_in
      .data_in_i (axi_rdata_i),
      .valid_in_i(axi_rvalid_i),

      // Simple interface for data_out
      .data_out_o (m_rdata_o),
      .valid_out_o(m_rready_o),

      .clk_i(clk_i),
      .rst_i(rst_i)
   );

   reg transfer_start, burst_i_start;

   wire [AXI_LEN_W-1:0] true_axi_arlen;
   reg [AXI_LEN_W-1:0] read_axi_len;
   assign axi_arlen_o = read_axi_len;

   wire [31:0] symbolsToRead;
   reg [31:0] symbolsRead;

   // TODO:
   // Read the text on the interface.
   // This is mostly a hack to make things work correct, but it is more logic than we actually need.
   // We can probably save a good deal of resources by simplifying this following the approach mentioned on the interface text.
   assign m_rlast_o = (read_state == 0 || (m_rready_o && (symbolsRead + 1) >= symbolsToRead));

   AXITransferController #(
      .AXI_ADDR_W(AXI_ADDR_W),
      .AXI_DATA_W(AXI_DATA_W),
      .AXI_LEN_W (AXI_LEN_W),
      .LEN_W     (LEN_W)
   ) read_controller (
      .address_i(m_raddr_i),
      .length_i (m_rlen_i),   // In bytes

      .transfer_start_i((read_state == 2'h0) && m_rvalid_i && burst_i_align_empty),
      .burst_start_i   ((read_state == 2'h2) && axi_arready_i && axi_arvalid_o),

      // Do not need them for read operation
      .initial_strb_o      (),
      .final_strb_o        (),
      .symbolsToRead_o     (symbolsToRead),
      .last_transfer_next_o(),

      .true_axi_axaddr_o(axi_araddr_o),

      // TODO: Register these signals to 
      .true_axi_axlen_o(true_axi_arlen),
      .last_transfer_o (read_last_transfer),

      .clk_i(clk_i),
      .rst_i(rst_i)
   );

   always @(posedge clk_i, posedge rst_i) begin
      if (rst_i) begin
         read_state   <= 0;
         arvalid      <= 0;
         read_axi_len <= 0;
         symbolsRead  <= 0;
      end else begin
         if(read_state > 0) begin
            if(m_rvalid_i && m_rready_o) begin
               symbolsRead <= symbolsRead + 1;
            end
         end

         case (read_state)
            2'h0: begin
               if (m_rvalid_i && burst_i_align_empty) begin
                  read_state <= 2'h1;
                  symbolsRead <= 0;
               end
            end
            2'h1: begin
               arvalid      <= 1'b1;
               read_state   <= 2'h2;
               read_axi_len <= true_axi_arlen;
            end
            2'h2: begin  // Write address set
               if (axi_arready_i) begin
                  arvalid    <= 1'b0;
                  read_state <= 2'h3;
               end
            end
            2'h3: begin
               if (axi_rvalid_i && axi_rready_o && axi_rlast_i) begin
                  if (read_last_transfer) begin
                     read_state <= 2'h0;
                  end else begin
                     read_state <= 2'h1;
                  end
               end
            end
         endcase
      end
   end

endmodule  // SimpleAXItoAXIRead
