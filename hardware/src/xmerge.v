`timescale 1ns / 1ps

module xmerge #(
   parameter ADDR_W   = 0,
   parameter DATA_W   = 32,
   parameter N_SLAVES = 2,
   parameter LEN_W    = 8
) (
   // Slaves
   input  [               N_SLAVES-1:0] s_valid_i,
   output [               N_SLAVES-1:0] s_ready_o,
   output [               N_SLAVES-1:0] s_last_o,
   input  [      ADDR_W * N_SLAVES-1:0] s_addr_i,
   input  [      DATA_W * N_SLAVES-1:0] s_wdata_i,
   input  [(DATA_W / 8) * N_SLAVES-1:0] s_wstrb_i,
   output [                 DATA_W-1:0] s_rdata_o,
   input  [     LEN_W * N_SLAVES - 1:0] s_len_i,

   // Write interface
   output reg                    m_wvalid_o,
   input                         m_wready_i,
   output reg [      ADDR_W-1:0] m_waddr_o,
   output reg [      DATA_W-1:0] m_wdata_o,
   output reg [(DATA_W / 8)-1:0] m_wstrb_o,
   output reg [       LEN_W-1:0] m_wlen_o,
   input                         m_wlast_i,

   // Read interface
   output reg              m_rvalid_o,
   input                   m_rready_i,
   output reg [ADDR_W-1:0] m_raddr_o,
   input      [DATA_W-1:0] m_rdata_i,
   output reg [ LEN_W-1:0] m_rlen_o,
   input                   m_rlast_i,

   input clk_i,
   input rst_i
);

   wire [      ADDR_W-1:0] st_addr [N_SLAVES-1:0];
   wire [      DATA_W-1:0] st_wdata[N_SLAVES-1:0];
   wire [(DATA_W / 8)-1:0] st_wstrb[N_SLAVES-1:0];
   wire [       LEN_W-1:0] st_len  [N_SLAVES-1:0];

   generate
      genvar g;
      for (g = 0; g < N_SLAVES; g = g + 1) begin : unpackSlave
         assign st_addr[g]  = s_addr_i[ADDR_W*g+:ADDR_W];
         assign st_wdata[g] = s_wdata_i[DATA_W*g+:DATA_W];
         assign st_wstrb[g] = s_wstrb_i[(DATA_W/8)*g+:(DATA_W/8)];
         assign st_len[g]   = s_len_i[LEN_W*g+:LEN_W];
      end
   endgenerate

   assign s_rdata_o = m_rdata_i;

   // If currently working and which interface it is servicing
   reg                            w_running;
   reg     [$clog2(N_SLAVES)-1:0] w_slave;
   reg                            r_running;
   reg     [$clog2(N_SLAVES)-1:0] r_slave;

   // If there is any request (any slave is asserting valid)
   reg                            w_req_valid;
   reg     [$clog2(N_SLAVES)-1:0] w_req;
   reg                            r_req_valid;
   reg     [$clog2(N_SLAVES)-1:0] r_req;

   integer                        i;
   always @* begin
      w_req_valid = 0;
      r_req_valid = 0;
      w_req       = 0;
      r_req       = 0;

      for (i = 0; i < N_SLAVES; i = i + 1) begin
         if (s_valid_i[i]) begin
            if (|st_wstrb[i]) begin
               w_req_valid = 1'b1;
               w_req       = i;
            end else begin
               r_req_valid = 1'b1;
               r_req       = i;
            end
         end
      end
   end

   wire r_transfer = (m_rvalid_o && m_rready_i);
   wire w_transfer = (m_wvalid_o && m_wready_i);

   always @(posedge clk_i, posedge rst_i) begin
      if (rst_i) begin
         w_running <= 0;
         w_slave   <= 0;
         r_running <= 0;
         r_slave   <= 0;
      end else begin
         if (!w_running && w_req_valid) begin
            w_running <= 1'b1;
            w_slave   <= w_req;
         end
         if (w_running && m_wlast_i && w_transfer) begin
            w_running <= 1'b0;
         end

         if (!r_running && r_req_valid) begin
            r_running <= 1'b1;
            r_slave   <= r_req;
         end
         if (r_running && m_rlast_i && r_transfer) begin
            r_running <= 1'b0;
         end
      end
   end

   assign s_ready_o = ((w_running && m_wready_i ? 1 : 0) << w_slave) | ((r_running && m_rready_i ? 1 : 0) << r_slave);
   assign s_last_o = ((w_running && m_wlast_i ? 1 : 0 ) << w_slave) | ((r_running && m_rlast_i ? 1 : 0 ) << r_slave);

   always @* begin
      m_rvalid_o = 0;
      m_raddr_o  = 0;
      m_rlen_o   = 0;

      m_wvalid_o = 0;
      m_waddr_o  = 0;
      m_wdata_o  = 0;
      m_wstrb_o  = 0;
      m_wlen_o   = 0;

      if (w_running) begin
         m_wvalid_o = s_valid_i[w_slave];
         m_waddr_o  = st_addr[w_slave];
         m_wdata_o  = st_wdata[w_slave];
         m_wstrb_o  = st_wstrb[w_slave];
         m_wlen_o   = st_len[w_slave];
      end

      if (r_running) begin
         m_rvalid_o = s_valid_i[r_slave];
         m_raddr_o  = st_addr[r_slave];
         m_rlen_o   = st_len[r_slave];
      end
   end

endmodule  // xmerge
