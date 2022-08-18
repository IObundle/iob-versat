`timescale 1ns / 1ps

module SimpleIOBToAxi 
  #(
    parameter DATA_W = 32
  )
  (
      output reg           s_ready,
      input                s_valid,
      input [31:0]         s_addr,
      output [DATA_W-1:0]  s_rdata,
      input [DATA_W-1:0]   s_wdata,
      input [DATA_W/8-1:0] s_wstrb,

      output            m_axi_awid,
      output reg [31:0] m_axi_awaddr,
      output [7:0]      m_axi_awlen,
      output [2:0]      m_axi_awsize,
      output [1:0]      m_axi_awburst,
      output            m_axi_awlock,
      output [3:0]      m_axi_awcache,
      output [2:0]      m_axi_awprot,
      output [3:0]      m_axi_awqos,
      output reg        m_axi_awvalid,
      input             m_axi_awready,

      //write
      output reg [31:0]     m_axi_wdata,
      output reg [32/8-1:0] m_axi_wstrb,
      output reg            m_axi_wlast,
      output reg            m_axi_wvalid,
      input                 m_axi_wready,

      //write response
      input      m_axi_bid,
      input      m_axi_bresp,
      input      m_axi_bvalid,
      output reg m_axi_bready,

      //address read
      output                        m_axi_arid,
      output reg [`DDR_ADDR_W-1:0]  m_axi_araddr,
      output [7:0]                  m_axi_arlen,
      output [2:0]                  m_axi_arsize,
      output [1:0]                  m_axi_arburst,
      output                        m_axi_arlock,
      output [3:0]                  m_axi_arcache,
      output [2:0]                  m_axi_arprot,
      output [3:0]                  m_axi_arqos,
      output reg                    m_axi_arvalid,
      input                         m_axi_arready,

      //read
      input        m_axi_rid,
      input [31:0] m_axi_rdata,
      input        m_axi_rresp,
      input        m_axi_rlast,
      input        m_axi_rvalid,
      output reg   m_axi_rready,

      input        clk,
      input        rst
  );

assign m_axi_awid = 1'b0;
assign m_axi_awlen = 8'h0;
assign m_axi_awsize = 3'b010;
assign m_axi_awburst = 2'b01;
assign m_axi_awlock = 1'b0;
assign m_axi_awcache = 4'b0000;
assign m_axi_awprot = 3'b000;
assign m_axi_awqos = 4'h0;

assign m_axi_arid = 1'b0;
assign m_axi_arlen = 8'h0;
assign m_axi_arsize = 3'b010;
assign m_axi_arburst = 2'b01;
assign m_axi_arlock = 1'b0;
assign m_axi_arcache = 4'b0000;
assign m_axi_arprot = 3'b000;
assign m_axi_arqos = 4'h0;

reg [4:0] state;

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      state <= 0;
      s_ready <= 1'b0;
      s_rdata <= 0;
      m_axi_awaddr <= 0;
      m_axi_wdata <= 0;
      m_axi_wstrb <= 0;
   end else begin
      s_ready <= 1'b0;
      
      case(state)
      4'h0: begin
         if(s_valid) begin
            if(|s_wstrb) begin
               state <= 4'h1;
               m_axi_awaddr <= s_addr;
               m_axi_wdata <= s_wdata;
               m_axi_wstrb <= s_wstrb;
            end else begin
               state <= 4'h4;
               m_axi_araddr <= s_addr;
            end
         end
      end
      4'h1: begin
         if(m_axi_awready) begin
            state <= 4'h2;
         end
      end
      4'h2: begin
         if(m_axi_wready) begin
            state <= 4'h3;
         end
      end
      4'h3: begin
         if(m_axi_bvalid) begin
            state <= 4'h0;
            s_ready <= 1'b1;
         end
      end    
      4'h4: begin
         if(m_axi_arready) begin
            state <= 4'h5;
         end
      end
      4'h5: begin
         if(m_axi_rvalid) begin
            state <= 4'h0;
            s_rdata <= m_axi_rdata;
            s_ready <= 1'b1;
         end
      end
      endcase
   end
end

always @*
begin
   m_axi_awvalid = 1'b0;
   m_axi_wvalid = 1'b0;
   m_axi_wlast = 1'b0;
   m_axi_bready = 1'b0;
   m_axi_arvalid = 1'b0;
   m_axi_rready = 1'b0;

   case(state)
      4'h1: begin
         m_axi_awvalid = 1'b1;
      end
      4'h2: begin
         m_axi_wvalid = 1'b1;
         m_axi_wlast = 1'b1;
      end
      4'h3: begin
         m_axi_bready = 1'b1;
      end
      4'h4: begin
         m_axi_arvalid = 1'b1;
      end
      4'h5: begin
         m_axi_rready = 1'b1;
      end
   endcase
end

endmodule

