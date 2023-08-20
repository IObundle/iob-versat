`timescale 1ns / 1ps

(* source *) module SignalMemWrite #(
         parameter MEM_INIT_FILE="none",
         parameter DATA_W = 32,
         parameter ADDR_W = 12,
         parameter PERIOD_W = 10,
         parameter AXI_ADDR_W = 32,
         parameter AXI_DATA_W = 32,
         parameter LEN_W = 8
      )
    (
   //control
   input                         clk,
   input                         rst,

   input                         running,   
   input                         run,

   output                        done,

   input                         databus_ready_0,
   output                        databus_valid_0,
   output[AXI_ADDR_W-1:0]        databus_addr_0,
   input [AXI_DATA_W-1:0]        databus_rdata_0,
   output [AXI_DATA_W-1:0]       databus_wdata_0,
   output [AXI_DATA_W/8-1:0]     databus_wstrb_0,
   output [LEN_W-1:0]            databus_len_0,
   input                         databus_last_0,

   //input / output data
   input [DATA_W-1:0]            in0, // Data
   input [DATA_W-1:0]            in1, // Signal

   // Config
   input [AXI_ADDR_W-1:0] ext_addr,

   // External memory
   output [ADDR_W-1:0]     ext_2p_addr_out_0,
   output reg [DATA_W-1:0] ext_2p_data_out_0,
   output reg              ext_2p_write_0,

   output [ADDR_W-1:0]     ext_2p_addr_in_0,
   input  [AXI_DATA_W-1:0] ext_2p_data_in_0,
   output                  ext_2p_read_0,

   input [31:0]                  delay0,
   input                         disabled,

   // Configuration
   output [ADDR_W-1:0]           stored
   );

assign databus_wstrb_0 = ~0;
assign databus_addr_0 = ext_addr;

reg [31:0] delay;

reg [ADDR_W-1:0] currentStored;
reg [ADDR_W-1:0] lastStored;

assign stored = lastStored;

reg pingPongState;

// Ping pong 
always @(posedge clk,posedge rst)
begin
   if(rst)
      pingPongState <= 0;
   else if(run && !disabled)
      pingPongState <= !pingPongState;
end

//assign ext_2p_addr_out_0 = pingPong ? {pingPongState,addr[ADDR_W-2:0]} : addr;
//assign ext_2p_write_0 = valid;

wire [ADDR_W-2:0] zeroAddress = 0;

reg [ADDR_W-1:0] port_0_address;
assign ext_2p_addr_out_0 = {pingPongState,port_0_address[ADDR_W-2:0]};

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      port_0_address <= 0;
      ext_2p_data_out_0 <= 0;
      ext_2p_write_0 <= 0;
      delay <= 0;      
      currentStored <= 0;
   end else if(run && !disabled) begin
      delay <= 0; // delay0;
      lastStored <= currentStored;
      currentStored <= 0;
      port_0_address <= 0;
      ext_2p_data_out_0 <= 0;
      ext_2p_write_0 <= 0;
   end else if(|delay) begin
      delay <= delay - 1;
   end else begin
      ext_2p_write_0 <= 0;
      if(|in1) begin
         currentStored <= currentStored + 1;
         port_0_address <= {!pingPongState,currentStored[ADDR_W-2:0]};
         ext_2p_data_out_0 <= in0;
         ext_2p_write_0 <= 1;
      end
   end
end

wire gen_ready;
reg writting;
assign done = (!writting);

reg [ADDR_W-2:0] counter;
wire [ADDR_W-1:0] gen_addr = !pingPongState ? {1'b1,counter} : {1'b0,counter};

wire gen_valid = (running && counter < stored[ADDR_W-2:0]);

assign databus_len_0 = (stored * 4);

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      counter <= 0;
   end else if(run && !disabled) begin
      counter <= 0;
      writting <= (currentStored != 0);
   end else if(running && !disabled) begin
      if(gen_valid && gen_ready) begin
         counter <= counter + 1;
      end
      if(databus_ready_0 && databus_valid_0 && databus_last_0) begin
         writting <= 1'b0;
      end
   end
end

wire m_valid;
wire read_en;
wire [ADDR_W-1:0] read_addr;
wire [AXI_DATA_W-1:0] read_data;

MemoryReader #(.ADDR_W(ADDR_W),.DATA_W(AXI_DATA_W))
reader(
   // Slave
   .s_valid(gen_valid),
   .s_ready(gen_ready),
   .s_addr(gen_addr),

   // Master
   .m_valid(m_valid),
   .m_ready(databus_ready_0),
   .m_addr(),
   .m_data(databus_wdata_0),

   // Connect to memory
   .mem_enable(read_en),
   .mem_addr(read_addr),
   .mem_data(read_data),

   .clk(clk),
   .rst(rst)
);

wire [ADDR_W-1:0] true_read_addr;
generate
if(AXI_DATA_W == 32) begin
assign true_read_addr = read_addr;
end   
else if(AXI_DATA_W == 64) begin
assign true_read_addr = (read_addr >> 1);      
end
else if(AXI_DATA_W == 128) begin
assign true_read_addr = (read_addr >> 2);      
end
else if(AXI_DATA_W == 256) begin
assign true_read_addr = (read_addr >> 3);      
end
else if(AXI_DATA_W == 512) begin
assign true_read_addr = (read_addr >> 4);      
end else begin
   initial begin $display("NOT IMPLEMENTED\n"); $finish(); end
end
endgenerate

assign databus_valid_0 = m_valid;

assign ext_2p_read_0 = read_en;
assign ext_2p_addr_in_0 = true_read_addr;
assign read_data = ext_2p_data_in_0;

endmodule
