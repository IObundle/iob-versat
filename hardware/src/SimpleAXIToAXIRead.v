`timescale 1ns / 1ps

module SimpleAXItoAXIRead #(
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

   `include "axi_m_read_port.vs"

   input clk_i,
   input rst_i
);

   localparam OFFSET_W = $clog2(AXI_DATA_W / 8);

   localparam [2:0] axi_size = (AXI_DATA_W == 16   ? 3'b001 : 
                             AXI_DATA_W == 32   ? 3'b010 :
                             AXI_DATA_W == 64   ? 3'b011 :
                             AXI_DATA_W == 128  ? 3'b100 :
                             AXI_DATA_W == 256  ? 3'b101 :
                             AXI_DATA_W == 512  ? 3'b110 : 
                             AXI_DATA_W == 1024 ? 3'b111 : 3'b000);

   // Read

   assign axi_arid_o    = 0;
   assign axi_arsize_o  = axi_size;
   assign axi_arburst_o = 'b01;  // INCR
   assign axi_arlock_o  = 'b0;
   assign axi_arcache_o = 'h2;
   assign axi_arprot_o  = 'b010;
   assign axi_arqos_o   = 'h0;

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

      .last_transfer_o(m_rlast_o),
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

   AXITransferController #(
      .AXI_ADDR_W(AXI_ADDR_W),
      .AXI_DATA_W(AXI_DATA_W),
      .AXI_LEN_W (AXI_LEN_W),
      .LEN_W     (LEN_W)
   ) read_controller (
      .address_i(m_raddr_i),
      .length_i (m_rlen_i),   // In bytes

      .transfer_start_i(read_state == 2'h0 && m_rvalid_i && burst_i_align_empty),
      .burst_start_i   (read_state == 2'h2 && axi_arready_i && axi_arvalid_o),

      // Do not need them for read operation
      .initial_strb_o      (),
      .final_strb_o        (),
      .symbolsToRead_o     (),
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
      end else begin
         case (read_state)
            2'h0: begin
               if (m_rvalid_i && burst_i_align_empty) begin
                  read_state <= 2'h1;
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
