`timescale 1ns / 1ps

module Buffer #(
         parameter ADDR_W = 6,
         parameter DATA_W = 32
      )
    (
    //control
    input                         clk,
    input                         rst,
    
   input                          running,
   input                          run,

   // External memory
   output [ADDR_W-1:0]     ext_2p_addr_out_0,
   output [ADDR_W-1:0]     ext_2p_addr_in_0,
   output                  ext_2p_write_0,
   output                  ext_2p_read_0,
   input  [DATA_W-1:0]     ext_2p_data_in_0,
   output [DATA_W-1:0]     ext_2p_data_out_0,

    //input / output data
    input [DATA_W-1:0]            in0,

    (* versat_latency = 1 *) output reg [DATA_W-1:0] out0,
    
    input [ADDR_W-1:0]     amount
    );

wire [ADDR_W:0] occupancy;
wire read_en = (running && occupancy >= {1'b0,amount});
wire write_en = (running && occupancy <= {1'b0,amount});

reg [DATA_W-1:0] inData;
wire [DATA_W-1:0] fifo_data;
iob_fifo_sync #(.W_DATA_W(DATA_W),.R_DATA_W(DATA_W),.ADDR_W(ADDR_W))
   fifo 
   (
      .rst_i(rst),
      .clk_i(clk),
      .cke_i(1'b1),
      .arst_i(1'b0),

      .level_o(occupancy), 

      //read port
      .r_data_o(fifo_data), 
      .r_empty_o(),
      .r_en_i(read_en),

      //write port
      .w_data_i(in0),
      .w_full_o(),
      .w_en_i(write_en),

      //write port
      .ext_mem_clk_o(),
      .ext_mem_w_en_o(ext_2p_write_0),
      .ext_mem_w_addr_o(ext_2p_addr_out_0),
      .ext_mem_w_data_o(ext_2p_data_out_0),
      //read port
      .ext_mem_r_en_o(ext_2p_read_0),
      .ext_mem_r_addr_o(ext_2p_addr_in_0),
      .ext_mem_r_data_i(ext_2p_data_in_0)
   );

always @(posedge clk,posedge rst)
begin
   if(rst)
      inData <= 0;
   else
      inData <= in0;
end

always @*
begin
   out0 = 0;

   if(amount == 0)
      out0 = inData;
   else
      out0 = fifo_data;
end

endmodule
