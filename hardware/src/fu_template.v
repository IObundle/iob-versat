`timescale 1ns/1ps
`include "versat.vh"
`include "stage.vh"
`include "fu_template.vh"

module fu_template 
  #(
    parameter N_FLOW_IN = 1,
    parameter N_FLOW_OUT = 1
    )

   (
    input                           clk,
    input                           rst,

                 //cpu interface 
    input                           valid,
    input [2:0]                     address,
    input [31:0]                    wdata,
    input                           wstrb,
    output reg [31:0]               rdata,
    output reg                      ready,
 
 //mem interface
    input [`REQ_W-1:0]              mem_in,
    output [`REQ_W-1:0]             mem_out,
 
 //flow interface
    input [N_FLOW_IN*`DATA_W-1:0]   flow_in,
    output [N_FLOW_OUT*`DATA_W-1:0] flow_out,

 //control
    input                           run,
    output                          done
 
    );

   ////////////////////////////////////////////////////////
   // Address decoder
   ////////////////////////////////////////////////////////

   reg                              sela_wr_en;
   reg                              selb_wr_en;
   reg                              fns_wr_en;
   
   // write
   always @* begin

      sela_wr_en = 1'b0;
      selb_wr_en = 1'b0;
      fns_wr_en = 1'b0;
      
      if(valid & wstrb)
        case (address)
          `FU_TEMPLATE_SELA: sela_wr_en = 1'b1;
          `FU_TEMPLATE_SELB: selb_wr_en = 1'b1;
          `FU_TEMPLATE_FNS: fns_wr_en = 1'b1;
          default:;
        endcase
   end // always @ *

   reg [`FU_TEMPLATE_SEL_W-1:0] sela, sela_shadow;
   reg [`FU_TEMPLATE_SEL_W-1:0] selb, selb_shadow;
   reg [`FU_TEMPLATE_FNS_W-1:0] fns, fns_shadow;
   
   always @(posedge clk) begin
      if(sela_wr_en)
        sela <= wdata[`FU_TEMPLATE_SEL_W-1:0];     
      if(selb_wr_en)
        selb <= wdata[`FU_TEMPLATE_SEL_W-1:0];
      if(fns_wr_en)
        fns <= wdata[`FU_TEMPLATE_FNS_W-1:0];
   end

   always @(posedge clk) begin
      if(run)
        sela_shadow <= sela;
      if(run)
        selb_shadow <= selb;
      if(fns_wr_en)
        fns_shadow <= fns;
   end

   //read
   always @* begin
      data_read_en = 1'b0;
      rdata = recv_buf_data;
      case (address)
        `FU_TEMPLATE_SELA: rdata = sela | `DATA_W'd'0;
        `FU_TEMPLATE_SELB: rdata = selb | `DATA_W'd'0;
        `FU_TEMPLATE_FNS: rdata = fns | `DATA_W'd'0;
        default:;
      endcase
   end

   // Input selection 
   xinmux # ( 
	.DATA_W(`DATA_W)
   ) muxa (
	.sel(sela),
	.data_in(flow_in),
	.data_out(`op_a)
	);
   
   xinmux # ( 
	.DATA_W(DATA_W)
   ) muxb (
	.sel(selb),
	.data_in(flow_in),
	.data_out(op_b)
	);


   // 

   //
   //instantiate fu core here
   //
   
endmodule

