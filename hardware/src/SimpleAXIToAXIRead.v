`timescale 1ns/1ps
`include "axi.vh"
`include "AXIInfo.vh"

module SimpleAXItoAXIRead #(
    parameter AXI_ADDR_W = 32,
    parameter AXI_DATA_W = 32,
    parameter AXI_LEN_W = 8,
    parameter AXI_ID_W = 4,
    parameter LEN_W = 8
  )
  (
    input  m_rvalid,
    output reg m_rready,
    input  [AXI_ADDR_W-1:0] m_raddr,
    output [AXI_DATA_W-1:0] m_rdata,
    input  [LEN_W-1:0] m_rlen,
    output m_rlast,

    `include "m_versat_axi_m_read_port.vh"

    input clk,
    input rst
  );

localparam OFFSET_W = calculate_AXI_OFFSET_W(AXI_DATA_W);

localparam [2:0] axi_size = (AXI_DATA_W == 16   ? 3'b001 : 
                             AXI_DATA_W == 32   ? 3'b010 :
                             AXI_DATA_W == 64   ? 3'b011 :
                             AXI_DATA_W == 128  ? 3'b100 :
                             AXI_DATA_W == 256  ? 3'b101 :
                             AXI_DATA_W == 512  ? 3'b110 : 
                             AXI_DATA_W == 1024 ? 3'b111 : 3'b000);

// Read

assign m_axi_arid = `AXI_ID_W'b0;
assign m_axi_arsize = axi_size;
assign m_axi_arburst = `AXI_BURST_W'b01; // INCR
assign m_axi_arlock = `AXI_LOCK_W'b0;
assign m_axi_arcache = `AXI_CACHE_W'h2;
assign m_axi_arprot = `AXI_PROT_W'b010;
assign m_axi_arqos = `AXI_QOS_W'h0;

assign m_axi_arvalid = arvalid;
assign m_axi_rready = (read_state == 2'h3);

reg arvalid,rready;

reg [1:0] read_state;

wire read_last_transfer;
wire burst_align_empty;
// Read
burst_align #(
    .AXI_DATA_W(AXI_DATA_W)
  ) aligner (
    .offset(m_raddr[OFFSET_W-1:0]),
    .start(read_state == 0),

    .burst_last(m_axi_rvalid && m_axi_rready && m_axi_rlast),
    .transfer_last(read_last_transfer),

    .last_transfer(m_rlast),
    .empty(burst_align_empty),

    // Simple interface for data_in
    .data_in(m_axi_rdata),
    .valid_in(m_axi_rvalid),

    // Simple interface for data_out
    .data_out(m_rdata),
    .valid_out(m_rready),

    .clk(clk),
    .rst(rst)
  );

reg transfer_start,burst_start;

wire [7:0] true_axi_arlen;

transfer_controller #(
   .AXI_ADDR_W(AXI_ADDR_W),
   .AXI_DATA_W(AXI_DATA_W),
   .LEN_W(LEN_W) 
   )
  read_controller
   (
      .address(m_raddr),
      .length(m_rlen), // In bytes

      .transfer_start(read_state == 2'h0 && m_rvalid && burst_align_empty),
      .burst_start(read_state == 2'h2 && m_axi_arready && m_axi_arvalid),

      // Do not need them for read operation
      .initial_strb(),
      .final_strb(),
      .symbolsToRead(),
      .last_transfer_next(),

      .true_axi_axaddr(m_axi_araddr),

      // TODO: Register these signals to 
      .true_axi_axlen(true_axi_arlen),
      .last_transfer(read_last_transfer),
   
      .clk(clk),
      .rst(rst)
   );

reg [7:0] read_axi_len;
assign m_axi_arlen = read_axi_len;

always @(posedge clk,posedge rst)
begin
  if(rst) begin
    read_state <= 0;
    arvalid <= 0;
    read_axi_len <= 0;
  end else begin
    case(read_state)
    2'h0: begin
      if(m_rvalid && burst_align_empty) begin
        read_state <= 2'h1;
      end
    end
    2'h1: begin
      arvalid <= 1'b1;
      read_state <= 2'h2;
      read_axi_len <= true_axi_arlen;
    end
    2'h2: begin // Write address set
      if(m_axi_arready) begin
        arvalid <= 1'b0;
        read_state <= 2'h3;
      end
    end
    2'h3: begin
      if(m_axi_rvalid && m_axi_rready && m_axi_rlast) begin
        if(read_last_transfer) begin      
          read_state <= 2'h0;
        end else begin
          read_state <= 2'h1;
        end
      end
    end
    endcase
  end
end


endmodule

