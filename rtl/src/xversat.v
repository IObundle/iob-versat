`timescale 1ns / 1ps

// top defines
`include "xversat.vh"
`include "xconfdefs.vh"


// FU defines              
`include "xmemdefs.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"
`include "xdivdefs.vh"

module xversat (
                 input                    clk,
                 input                    rst,

                 //control interface
                 input                    ctr_valid,
                 input [`ADDR_W-1:0]      ctr_addr,
                 input                    ctr_we,
                 input [`DATA_W-1:0]      ctr_data_in,
                 output reg [`DATA_W-1:0] ctr_data_out,

                 //data interface
                 input                    data_valid,
                 input [`ADDR_W-3:0]      data_addr,
                 input                    data_we,
                 input [`DATA_W-1:0]      data_data_in,
                 output reg [`DATA_W-1:0] data_data_out
                 
                 );

   //Verilator ignores the fact that different array elements
   //are used, so use verilator lint_off UNOPTFLAT

   //data buses for each versat   
   wire [`DATABUS_W-1:0] stage_databus [`nSTAGE:0];

   //
   // CONTROL INTERFACE ADDRESS DECODER
   //

   //select control or data from MSB
   wire               ctr_control_valid = ctr_addr[`ADDR_W-1:0];
   wire               ctr_data_valid = ~ctr_addr[`ADDR_W-1:0];

   //select stage
   reg [`nSTAGE-1:0] stage_ctr_valid;
   always @ * begin
     integer j;
     ctr_stage_valid = `nSTAGE'b0;
       for(j=0; j<`nSTAGE; j=j+1)
          if (ctr_addr[`ADDR_W-2 -: `nSTAGE_W] == j[`nSTAGE_W-1:0])
            ctr_stage_valid[j] = ctr_valid;
   end 


   //WRITE to run address 
   wire               run = ctr_valid & ctr_data_in[0];

   //READ
   wire [`nSTAGE-1:0] status;
   reg [`DATA_W-1:0] ctr_stage_ctr_data_out;

   assign ctr_data_out = ctr_control_valid? {{`DATA_W-`nSTAGE{1'b0}},status}: ctr_stage_ctr_data_out;
   
   //select stage ctr data
   reg [`DATA_W-1:0] stage_ctr_data_out [`nSTAGE-1:0];
   always @ * begin
      integer j;
      ctr_stage_ctr_data_out = `DATA_W'd0;
      for(j=0; j<`nSTAGE; j=j+1)
         if (stage_ctr_valid[j])
           ctr_stage_ctr_data_out = stage_ctr_data_out[j];
   end




   
   //
   // DATA INTERFACE ADDRESS DECODER
   //

   //select stage
   reg [`nSTAGE-1:0] stage_data_valid;
   always @ * begin
     integer j;
     stage_data_valid = `nSTAGE'b0;
       for(j=0; j<`nSTAGE; j=j+1)
          if (data_addr[`ADDR_W-2 -: `nSTAGE_W] == j[`nSTAGE_W-1:0])
            stage_data_valid[j] = data_valid;
   end 


   //READ

   //select stage data data
   reg [`DATA_W-1:0] stage_data_data_out [`nSTAGE-1:0];
   always @ * begin
      integer j;
      data_out = `DATA_W'd0;
      for(j=0; j<`nSTAGE; j=j+1) begin
        if (data_stage_valid[j])
          data_out = stage_data_data_out[j];
      end
   end




 

   //
   // DATA INTERFACE ADDRESS DECODER
   //


   //
   // INSTANTIATE THE VERSAT STAGES
   //
   
   genvar i;
   generate
      for (i=0; i < `nSTAGE; i=i+1) begin : versat_array
        xstage stage (
                      .clk(clk),
                      .rst(rst),
                      .run(run),
                      .done(status[i]),

                      //control interface
                      .ctr_valid(stage_ctr_valid[i]),
                      .ctr_we(ctr_we),
                      .ctr_addr(ctr_addr[`ADDR_W-`nSTAGE_W-2:0]),
                      .ctr_data_in(ctr_data_in),
                      .ctr_data_out(stage_ctr_data_out[i]),
                      
                      //data  interface
                      .data_valid(stage_data_valid[i]),
                      .data_we(data_we),
                      .data_addr(data_addr[`ADDR_W-`nSTAGE-2:0]),
                      .data_data_in(data_data_in),
                      .data_data_out(stage_data_data_out[i]),
                      
                      //flow interface
                      .flow_in(stage_databus[i][`DATABUS_W-1:0]),
                      .flow_out(stage_databus[i+1][`DATABUS_W-1:0])
                      );
      end
    endgenerate


   //connect last stage back to first stage: ring topology
   assign stage_databus[0] = stage_databus [`nSTAGE];     

endmodule

