`timescale 1ns / 1ps

// Joins two simple write only interfaces into a single one.
// No pipelining for now and no backpreassure. Need to figure out how we actually want this before implementing more logic here
// This is mostly intended to allow DMA and CPU access to Versat address space.
// DMA only needs to write currently, so no point in doing anything complex right now

module JoinTwoSimple #(
      parameter DATA_W = 32,
      parameter ADDR_W = 32      
   ) (
      input                   m0_valid,
      input [DATA_W-1:0]      m0_data,
      input [ADDR_W-1:0]      m0_addr,
      input [(DATA_W/8)-1:0]  m0_wstrb,

      input                   m1_valid,
      input [DATA_W-1:0]      m1_data,
      input [ADDR_W-1:0]      m1_addr,
      input [(DATA_W/8)-1:0]  m1_wstrb,

      output                  s_valid,
      output [DATA_W-1:0]     s_data,
      output [ADDR_W-1:0]     s_addr,
      output [(DATA_W/8)-1:0] s_wstrb
   );

assign s_valid = (m0_valid | m1_valid);
assign s_data =  (m0_valid ? m0_data  : m1_data);
assign s_addr =  (m0_valid ? m0_addr  : m1_addr);
assign s_wstrb = (m0_valid ? m0_wstrb : m1_wstrb);

endmodule