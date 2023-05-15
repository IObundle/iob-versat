`timescale 1ns/1ps
`include "axi.vh"
`include "system.vh"
`include "iob_lib.vh"
`include "iob_intercon.vh"
`include "iob_versat.vh"

`include "versat_defs.vh"

module iob_versat
  # (//the below parameters are used in cpu if includes below
    	parameter AXI_ADDR_W = 32,
      parameter AXI_DATA_W = 32,
      parameter AXI_ID_W = 4,
      parameter ADDR_W = `VERSAT_ADDR_W, //NODOC Address width
    	parameter DATA_W = `VERSAT_RDATA_W, //NODOC CPU data width
    	parameter WDATA_W = `VERSAT_WDATA_W //NODOC CPU data width
    )
  	(
 		`include "iob_s_if.vh"

    `ifdef VERSAT_EXTERNAL_MEMORY
    `include "versat_external_memory_port.vh"
    `endif

   `ifdef VERSAT_IO
      `include "m_versat_axi_m_port.vh"
   `endif

   input clk,
   input rst
	);

`ifdef VERSAT_IO
wire [`nIO-1:0]               m_databus_ready;
wire [`nIO-1:0]               m_databus_valid;
wire [`nIO*`IO_ADDR_W-1:0]    m_databus_addr;
wire [`DATAPATH_W-1:0]        m_databus_rdata;
wire [`nIO*`DATAPATH_W-1:0]   m_databus_wdata;
wire [`nIO*`DATAPATH_W/8-1:0] m_databus_wstrb;
wire [`nIO*8-1:0]             m_databus_len;
wire [`nIO-1:0]               m_databus_last;

wire w_ready,w_valid;
wire [`IO_ADDR_W-1:0]    w_addr;
wire [`DATAPATH_W-1:0]   w_data;
wire [`DATAPATH_W/8-1:0] w_strb;
wire [7:0]               w_len;

wire r_ready,r_valid;
wire [`IO_ADDR_W-1:0]    r_addr;
wire [`DATAPATH_W-1:0]   r_data;
wire [7:0]               r_len;

wire w_last,r_last;

xmerge #(.N_SLAVES(`nIO),.ADDR_W(`IO_ADDR_W),.DATA_W(`DATAPATH_W)) merge(
  .s_valid(m_databus_valid),
  .s_ready(m_databus_ready),
  .s_addr(m_databus_addr),
  .s_wdata(m_databus_wdata),
  .s_wstrb(m_databus_wstrb),
  .s_rdata(m_databus_rdata),
  .s_len(m_databus_len),
  .s_last(m_databus_last),
  
  .m_wvalid(w_valid),
  .m_wready(w_ready),
  .m_waddr(w_addr),
  .m_wdata(w_data),
  .m_wstrb(w_strb),
  .m_wlen(w_len),
  .m_wlast(w_last),

  .m_rvalid(r_valid),
  .m_rready(r_ready),
  .m_raddr(r_addr),
  .m_rdata(r_data),
  .m_rlen(r_len),
  .m_rlast(r_last),

  .clk(clk),
  .rst(rst)
);

SimpleAXItoAXI #(
    .ADDR_W(AXI_ADDR_W),
    .DATA_W(DATA_W),
    .AXI_ID_W(AXI_ID_W)
  ) simpleToAxi(

  .m_wvalid(w_valid),
  .m_wready(w_ready),
  .m_waddr(w_addr),
  .m_wdata(w_data),
  .m_wstrb(w_strb),
  .m_wlen(w_len),
  .m_wlast(w_last),
  
  .m_rvalid(r_valid),
  .m_rready(r_ready),
  .m_raddr(r_addr),
  .m_rdata(r_data),
  .m_rlen(r_len),
  .m_rlast(r_last),

  `include "m_versat_axi_read_portmap.vh"
  `include "m_versat_axi_write_portmap.vh"

  .clk(clk),
  .rst(rst)
);

`endif

versat_instance #(.ADDR_W(ADDR_W),.DATA_W(DATA_W)) xversat(
      .valid(valid),
      .wstrb(wstrb),
      .addr(address),
      .rdata(rdata),
      .wdata(wdata),
      .ready(ready),

`ifdef VERSAT_EXTERNAL_MEMORY
      `include "versat_external_memory_portmap.vh"
`endif

`ifdef VERSAT_IO
      .m_databus_ready(m_databus_ready),
      .m_databus_valid(m_databus_valid),
      .m_databus_addr(m_databus_addr),
      .m_databus_rdata(m_databus_rdata),
      .m_databus_wdata(m_databus_wdata),
      .m_databus_wstrb(m_databus_wstrb),
      .m_databus_len(m_databus_len),
      .m_databus_last(m_databus_last),
`endif

      .clk(clk),
      .rst(rst)
); 

endmodule

module xmerge #(
    parameter ADDR_W = 0,
    parameter DATA_W = 32,
    parameter N_SLAVES = 2
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
    input [8 * N_SLAVES - 1:0] s_len,

    // Write interface
    output reg m_wvalid,
    input  m_wready,
    output reg [ADDR_W-1:0] m_waddr,
    output reg [DATA_W-1:0] m_wdata,
    output reg [(DATA_W / 8)-1:0] m_wstrb,
    output reg [7:0] m_wlen,
    input  m_wlast,

    // Read interface
    output reg m_rvalid,
    input  m_rready,
    output reg [ADDR_W-1:0] m_raddr,
    input [DATA_W-1:0] m_rdata,
    output reg [7:0] m_rlen,
    input  m_rlast,

    input clk,
    input rst
  );

wire [ADDR_W-1:0] st_addr[N_SLAVES-1:0];
wire [DATA_W-1:0] st_wdata[N_SLAVES-1:0];
wire [(DATA_W / 8)-1:0] st_wstrb[N_SLAVES-1:0];
wire [7:0] st_len[N_SLAVES-1:0];

generate
genvar g;
for(g = 0; g < N_SLAVES; g = g + 1)
begin
  assign st_addr[g] = s_addr[ADDR_W * g +: ADDR_W];
  assign st_wdata[g] = s_wdata[DATA_W * g +: DATA_W];
  assign st_wstrb[g] = s_wstrb[(DATA_W/8) * g +: (DATA_W/8)];
  assign st_len[g] = s_len[8 * g +: 8];
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

module SimpleAXItoAXI #(
    parameter AXI_ADDR_W = 32,
    parameter AXI_DATA_W = 32,
    parameter AXI_ID_W = 4,
    parameter ADDR_W = 0,
    parameter DATA_W = 32
  )
  (
    input  m_wvalid,
    output reg m_wready,
    input  [ADDR_W-1:0] m_waddr,
    input  [DATA_W-1:0] m_wdata,
    input  [(DATA_W / 8)-1:0] m_wstrb,
    input  [7:0] m_wlen,
    output m_wlast,
    
    input  m_rvalid,
    output reg m_rready,
    input  [ADDR_W-1:0] m_raddr,
    output [DATA_W-1:0] m_rdata,
    input  [7:0] m_rlen,
    output m_rlast,

    `include "m_versat_axi_m_port.vh"

    input clk,
    input rst
  );

assign m_axi_arid = `AXI_ID_W'b0;
assign m_axi_araddr = m_raddr;
assign m_axi_arlen = m_rlen;
assign m_axi_arsize = 2;
assign m_axi_arburst = `AXI_BURST_W'b01; // INCR
assign m_axi_arlock = `AXI_LOCK_W'b0;
assign m_axi_arcache = `AXI_CACHE_W'h2;
assign m_axi_arprot = `AXI_PROT_W'b010;
assign m_axi_arqos = `AXI_QOS_W'h0;

// Address write constants
assign m_axi_awid = `AXI_ID_W'b0;
assign m_axi_awaddr = m_waddr;
assign m_axi_awlen = m_wlen;
assign m_axi_awsize = 2;
assign m_axi_awburst = `AXI_BURST_W'b01; // INCR
assign m_axi_awlock = `AXI_LOCK_W'b0;
assign m_axi_awcache = `AXI_CACHE_W'h2;
assign m_axi_awprot = `AXI_PROT_W'b010;
assign m_axi_awqos = `AXI_QOS_W'h0;

assign m_axi_wdata = m_wdata;
assign m_axi_wstrb = m_wstrb;
assign m_rdata = m_axi_rdata;

assign m_axi_bready = 1'b1; // We ignore write response

reg awvalid,arvalid,wvalid,rready,wlast;
assign m_axi_awvalid = awvalid;
assign m_axi_arvalid = arvalid;
assign m_axi_wvalid = wvalid;
assign m_axi_rready = rready;
assign m_axi_wlast = wlast;

assign m_wlast = wlast;
assign m_rlast = m_axi_rlast;

reg [7:0] counter;
reg [2:0] state;
always @(posedge clk,posedge rst)
begin
  if(rst) begin
    state <= 0;
    awvalid <= 0;
    arvalid <= 0;
    counter <= 0;
  end else begin
    case(state)
    3'h0: begin
      if(m_wvalid) begin
        awvalid <= 1'b1;
        state <= 3'h1;
      end else if(m_rvalid) begin
        arvalid <= 1'b1;
        state <= 3'h3;
      end
    end
    3'h1: begin // Write address set
      if(m_axi_awready) begin
        awvalid <= 1'b0;
        state <= 3'h2;
      end
    end
    3'h2: begin
      if(m_axi_wvalid && m_axi_wready) begin
        counter <= counter + 1;
        if(wlast) begin
          state <= 3'h0;
          counter <= 0;
        end
      end
    end
    3'h3: begin // Write address set
      if(m_axi_arready) begin
        arvalid <= 1'b0;
        state <= 3'h4;
      end
    end
    3'h4: begin
      if(m_axi_rvalid && m_axi_rready && m_axi_rlast) begin
        state <= 3'h0;
      end
    end
    endcase
  end
end

always @*
begin
  wvalid = 1'b0;
  m_wready = 1'b0;
  wlast = 1'b0;
  rready = 1'b0;
  m_rready = 1'b0;

  if(state == 3'h2) begin
    wvalid = m_wvalid;
    m_wready = m_axi_wready;
    
    if(counter == m_wlen)
      wlast = 1'b1;
  end

  if(state == 3'h4) begin
    rready = m_rvalid;
    m_rready = m_axi_rvalid;
  end
end


/*
reg [7:0] counter;
reg [2:0] wstate;
always @(posedge clk,posedge rst)
begin
  if(rst) begin
    wstate <= 0;
    awvalid <= 0;
    counter <= 0;
  end else begin
    case(wstate)
    3'h0: begin
      if(m_wvalid) begin
        awvalid <= 1'b1;
        wstate <= 3'h1;
      end
    end
    3'h1: begin // Write address set
      if(m_axi_awready) begin
        awvalid <= 1'b0;
        wstate <= 3'h2;
      end
    end
    3'h2: begin
      if(m_axi_wvalid && m_axi_wready) begin
        counter <= counter + 1;
        if(wlast) begin
          wstate <= 3'h0;
          counter <= 0;
        end
      end
    end
    endcase
  end
end

always @*
begin
  wvalid = 1'b0;
  m_wready = 1'b0;
  wlast = 1'b0;

  if(wstate == 3'h2) begin
    wvalid = m_wvalid;
    m_wready = m_axi_wready;
    
    if(counter == m_wlen)
      wlast = 1'b1;
  end
end

reg [2:0] rstate;
always @(posedge clk,posedge rst)
begin
  if(rst) begin
    rstate <= 0;
    arvalid <= 0;
  end else begin
    case(rstate)
    3'h0: begin
      if(m_rvalid) begin
        arvalid <= 1'b1;
        rstate <= 3'h1;
      end
    end
    3'h1: begin // Write address set
      if(m_axi_arready) begin
        arvalid <= 1'b0;
        rstate <= 3'h2;
      end
    end
    3'h2: begin
      if(m_axi_rvalid && m_axi_rready && m_axi_rlast) begin
        rstate <= 3'h0;
      end
    end
    endcase
  end
end

always @*
begin
  rready = 1'b0;
  m_rready = 1'b0;

  if(rstate == 3'h2) begin
    rready = m_rvalid;
    m_rready = m_axi_rvalid;
  end
end
*/

endmodule 

module skid_control(
    input in_valid,
    output reg in_ready,
    output in_transfer,

    output reg out_valid,
    input out_ready,
    output out_transfer,

    input valid,
    input enable_transfer,

    output reg store_data,
    output reg use_stored_data,

    input clk,
    input rst  
  );

assign in_transfer = (in_valid && in_ready);
assign out_transfer = (out_valid && out_ready && enable_transfer);

reg has_stored;

always @(posedge clk,posedge rst)
begin
  if(rst) begin
    has_stored <= 1'b0;
    in_ready <= 1'b1;
  end else begin
    case({!enable_transfer,has_stored,in_valid,out_ready})
    4'b0000:begin
      in_ready <= 1'b1;
    end
    4'b0001:begin
      in_ready <= 1'b1;
    end
    4'b0010:begin
      has_stored <= 1'b1;
      in_ready <= 1'b0;
    end
    4'b0011:begin
      in_ready <= 1'b1;
    end
    4'b0100:begin
      in_ready <= 1'b0;
    end
    4'b0101:begin
      has_stored <= 1'b0;
      in_ready <= 1'b1;
    end
    4'b0110:begin
      in_ready <= 1'b0;
    end
    4'b0111:begin
      in_ready <= 1'b1;
    end
    4'b1000:begin
      in_ready <= 1'b1;
    end
    4'b1001:begin
      in_ready <= 1'b1;
    end
    4'b1010:begin
      has_stored <= 1'b1;
      in_ready <= 1'b0;
    end
    4'b1011:begin
      has_stored <= 1'b1;
      in_ready <= 1'b0;
    end
    4'b1100:begin
      in_ready <= 1'b0;
    end
    4'b1101:begin 
      in_ready <= 1'b0;
    end
    4'b1110:begin
      in_ready <= 1'b0;
    end
    4'b1111:begin
      in_ready <= 1'b0;
    end
    endcase
  end
end

always @*
begin
  out_valid = 1'b0;
  store_data = 1'b0;
  use_stored_data = has_stored;

  case({!enable_transfer,has_stored,in_valid,out_ready})
  4'b0000:begin
  end
  4'b0001:begin
  end
  4'b0010:begin
    out_valid = valid;
  end
  4'b0011:begin
    out_valid = valid;
  end
  4'b0100:begin
    out_valid = valid;
  end
  4'b0101:begin
    out_valid = valid;
  end
  4'b0110:begin
    out_valid = valid;
  end
  4'b0111:begin
    out_valid = valid;
  end
  4'b1000:begin
  end
  4'b1001:begin
  end
  4'b1010:begin
    out_valid = valid;
  end
  4'b1011:begin
    out_valid = valid;
  end
  4'b1100:begin
    out_valid = valid;
  end
  4'b1101:begin
    out_valid = valid;
  end
  4'b1110:begin
    out_valid = valid;
  end
  4'b1111:begin
    out_valid = valid;
  end
  endcase  
end

endmodule

module pipeline_control(
    input in_valid,
    output in_ready,
    output in_transfer,

    output reg out_valid,
    input out_ready,
    output out_transfer,

    input clk,
    input rst  
  );

assign in_transfer = (in_valid && in_ready);
assign out_transfer = (out_valid && out_ready);

always @(posedge clk,posedge rst)
begin
  if(rst) begin
    out_valid <= 0;
  end else begin
    out_valid <= in_valid;
  end
end

assign in_ready = out_ready;

endmodule

