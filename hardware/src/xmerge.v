`timescale 1ns/1ps

`default_nettype none
module xmerge #(
    parameter ADDR_W = 0,
    parameter DATA_W = 32,
    parameter N_SLAVES = 2,
    parameter LEN_W = 8
  )
  (
    // Slaves
    input [N_SLAVES-1:0] s_valid,
    output [N_SLAVES-1:0] s_ready,
    output [N_SLAVES-1:0] s_last, 
    input [ADDR_W * N_SLAVES-1:0] s_addr,
    input [DATA_W * N_SLAVES-1:0] s_wdata,
    input [(DATA_W / 8) * N_SLAVES-1:0] s_wstrb,
    output [DATA_W-1:0] s_rdata,
    input [LEN_W * N_SLAVES - 1:0] s_len,

    // Write interface
    output reg m_wvalid,
    input  m_wready,
    output reg [ADDR_W-1:0] m_waddr,
    output reg [DATA_W-1:0] m_wdata,
    output reg [(DATA_W / 8)-1:0] m_wstrb,
    output reg [LEN_W-1:0] m_wlen,
    input  m_wlast,

    // Read interface
    output reg m_rvalid,
    input  m_rready,
    output reg [ADDR_W-1:0] m_raddr,
    input [DATA_W-1:0] m_rdata,
    output reg [LEN_W-1:0] m_rlen,
    input  m_rlast,

    input clk,
    input rst
  );

wire [ADDR_W-1:0] st_addr[N_SLAVES-1:0];
wire [DATA_W-1:0] st_wdata[N_SLAVES-1:0];
wire [(DATA_W / 8)-1:0] st_wstrb[N_SLAVES-1:0];
wire [LEN_W-1:0] st_len[N_SLAVES-1:0];

generate
genvar g;
for(g = 0; g < N_SLAVES; g = g + 1)
begin
  assign st_addr[g] = s_addr[ADDR_W * g +: ADDR_W];
  assign st_wdata[g] = s_wdata[DATA_W * g +: DATA_W];
  assign st_wstrb[g] = s_wstrb[(DATA_W/8) * g +: (DATA_W/8)];
  assign st_len[g] = s_len[LEN_W * g +: LEN_W];
end
endgenerate

assign s_rdata = m_rdata;

// If currently working and which interface it is servicing
reg w_running;
reg [$clog2(N_SLAVES)-1:0] w_slave;
reg r_running;
reg [$clog2(N_SLAVES)-1:0] r_slave;

// If there is any request (any slave is asserting valid)
reg w_req_valid;
reg [$clog2(N_SLAVES)-1:0] w_req;
reg r_req_valid;
reg [$clog2(N_SLAVES)-1:0] r_req;

integer i;
always @*
begin
  w_req_valid = 0;
  r_req_valid = 0;
  w_req = 0;
  r_req = 0;

  for(i = 0; i < N_SLAVES; i = i + 1)
  begin
    if(s_valid[i]) begin
      if(|st_wstrb[i]) begin
        w_req_valid = 1'b1;
        w_req = i;
      end else begin
        r_req_valid = 1'b1;
        r_req = i;
      end
    end
  end
end

wire r_transfer = (m_rvalid && m_rready);
wire w_transfer = (m_wvalid && m_wready);

always @(posedge clk,posedge rst)
begin
  if(rst) begin
    w_running <= 0;
    w_slave <= 0;
    r_running <= 0;
    r_slave <= 0;
  end else begin
    if(!w_running && w_req_valid) begin
      w_running <= 1'b1;
      w_slave <= w_req;
    end 
    if(w_running && m_wlast && w_transfer) begin
      w_running <= 1'b0;
    end

    if(!r_running && r_req_valid) begin
      r_running <= 1'b1;
      r_slave <= r_req;
    end 
    if(r_running && m_rlast && r_transfer) begin
      r_running <= 1'b0;
    end
  end
end

assign s_ready = ((w_running && m_wready ? 1 : 0) << w_slave) | ((r_running && m_rready ? 1 : 0) << r_slave);
assign s_last = ((w_running && m_wlast ? 1 : 0 ) << w_slave) | ((r_running && m_rlast ? 1 : 0 ) << r_slave);

always @*
begin
  m_rvalid = 0; 
  m_raddr = 0;
  m_rlen = 0;
 
  m_wvalid = 0; 
  m_waddr = 0;
  m_wdata = 0;
  m_wstrb = 0;
  m_wlen = 0;
  
  if(w_running) begin
    m_wvalid = s_valid[w_slave];
    m_waddr = st_addr[w_slave];
    m_wdata = st_wdata[w_slave];
    m_wstrb = st_wstrb[w_slave];
    m_wlen = st_len[w_slave];
  end 

  if(r_running) begin
    m_rvalid = s_valid[r_slave];
    m_raddr = st_addr[r_slave];
    m_rlen = st_len[r_slave];
  end
end

endmodule 