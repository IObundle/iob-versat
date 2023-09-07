`timescale 1ns/1ps
`include "axi.vh"
`include "AXIInfo.vh"

module SimpleAXItoAXIWrite #(
    parameter AXI_ADDR_W = 32,
    parameter AXI_DATA_W = 32,
    parameter AXI_LEN_W = 8,
    parameter AXI_ID_W = 4,
    parameter LEN_W = 8
  )
  (
    input  m_wvalid,
    output reg m_wready,
    input  [AXI_ADDR_W-1:0] m_waddr,
    input  [AXI_DATA_W-1:0] m_wdata,
    input  [(AXI_DATA_W / 8)-1:0] m_wstrb,
    input  [LEN_W-1:0] m_wlen,
    output reg m_wlast,

    `include "m_versat_axi_m_write_port.vh"

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

reg [2:0] state;
wire burst_ready_in;

reg flush_burst_split;
wire data_valid = m_wvalid && m_wready;

reg [OFFSET_W-1:0] stored_offset;

burst_split #(.DATA_W(AXI_DATA_W)) split(
        .offset(stored_offset),

        .data_in(m_wdata),
        .data_valid(data_valid),

        // Simple interface for data_out
        .data_out(m_axi_wdata),

        .clk(clk),
        .rst(rst)
    );

wire write_last_transfer;
wire write_last_transfer_next;

wire [7:0] true_axi_awlen;
wire [31:0] symbolsToRead_next;
reg [31:0] symbolsToRead;

reg [(AXI_DATA_W/8)-1:0] initial_strb,final_strb;

transfer_controller #(
   .AXI_ADDR_W(AXI_ADDR_W),
   .AXI_DATA_W(AXI_DATA_W),
   .LEN_W(LEN_W) 
   )
   write_controller
   (
      .address(m_waddr),
      .length(m_wlen), // In bytes

      .transfer_start(state == 3'h0 && m_wvalid),
      .burst_start(state == 3'h2 && m_axi_awready && m_axi_awvalid),

      .initial_strb(initial_strb),
      .final_strb(final_strb),

      .symbolsToRead(symbolsToRead_next),

      .true_axi_axaddr(m_axi_awaddr),

      // TODO: Register these signals to 
      .true_axi_axlen(true_axi_awlen),
      .last_transfer(write_last_transfer),
      .last_transfer_next(write_last_transfer_next),
   
      .clk(clk),
      .rst(rst)
   );

// Address write constants
assign m_axi_awid = `AXI_ID_W'b0;
assign m_axi_awsize = axi_size;
assign m_axi_awburst = `AXI_BURST_W'b01; // INCR
assign m_axi_awlock = `AXI_LOCK_W'b0;
assign m_axi_awcache = `AXI_CACHE_W'h2;
assign m_axi_awprot = `AXI_PROT_W'b010;
assign m_axi_awqos = `AXI_QOS_W'h0;

reg [AXI_DATA_W/8-1:0] wstrb;
//assign m_axi_wdata = m_wdata;
assign m_axi_wstrb = wstrb;

assign m_axi_bready = 1'b1; // We ignore write response

reg awvalid,wvalid;
assign m_axi_awvalid = awvalid;
assign m_axi_wvalid = wvalid;

reg m_axi_last;
assign m_axi_wlast = m_axi_last;

reg [7:0] write_axi_len;
assign m_axi_awlen = write_axi_len;

reg first_transfer;

reg [7:0] counter;
reg [31:0] full_counter;
reg [31:0] read_counter;
always @(posedge clk,posedge rst)
begin
  if(rst) begin
    state <= 0;
    awvalid <= 0;
    counter <= 0;
    full_counter <= 0;
    read_counter <= 0;
    write_axi_len <= 0;
    symbolsToRead <= 0;
    stored_offset <= 0;
    first_transfer <= 0;
    wstrb <= 0;
  end else begin
    case(state)
    3'h0: begin // Wait one cycle for transfer controller to calculate things.
      if(m_wvalid) begin
        //awvalid <= 1'b1;
        stored_offset <= m_waddr[OFFSET_W-1:0];
        state <= 3'h1;
        first_transfer <= 1'b1;
      end
    end
    3'h1: begin // Save values that change 
      symbolsToRead <= symbolsToRead_next;
      write_axi_len <= true_axi_awlen;
      awvalid <= 1'b1;
      state <= 3'h2;
    end
    3'h2: begin // Write address set
      if(m_axi_awready) begin
        awvalid <= 1'b0;
        state <= 3'h4;
        if(first_transfer) begin
           first_transfer <= 1'b0;
           wstrb <= initial_strb;
        end else begin
           wstrb <= ~0;
           if(write_axi_len == 8'h0 && write_last_transfer_next) begin
              wstrb <= final_strb;
           end
        end
      end
    end
    3'h4: begin
      if(m_axi_wvalid && m_axi_wready) begin
         read_counter <= read_counter + 1;
         counter <= counter + 1;

         if((counter + 1 == m_axi_awlen) && write_last_transfer) begin
            wstrb <= final_strb;
         end else begin
            wstrb <= ~0;      
         end

         full_counter <= full_counter + 1;
         if(m_axi_last) begin
            counter <= 0;
            if(write_last_transfer) begin
               full_counter <= 0;
               read_counter <= 0;
               symbolsToRead <= 0;
               write_axi_len <= 0;
               state <= 3'h0;
            end else begin
               state <= 3'h1;
            end
         end
      end
    end
    endcase
  end
end

always @*
begin
   m_wready = 1'b0;
   m_wlast = 1'b0;
   flush_burst_split = 1'b0;
   m_axi_last = 1'b0;
   wvalid = 1'b0;

   if(m_axi_wvalid && m_axi_wready && m_axi_wlast && write_last_transfer) begin
      m_wlast = 1'b1;
   end
   /*
   if(read_counter + 1 >= symbolsToRead) begin
      m_wlast = 1'b1;
   end
   */

   if(full_counter == symbolsToRead) begin
      flush_burst_split = 1'b1;
   end

   if(state == 3'h3) begin
      m_wready = 1'b1;
      wvalid = 1'b1;
   end

   if(state == 3'h4) begin
      m_wready = m_axi_wready;
      wvalid = m_wvalid || flush_burst_split;   

   if(counter == m_axi_awlen)
      m_axi_last = 1'b1;
  end
end

endmodule

