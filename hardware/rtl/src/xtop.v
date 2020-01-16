`timescale 1ns / 1ps       
                           
// top defines             
`include "xdefs.vh"        
`include "xctrldefs.vh"    
`include "xconfdefs.vh"    
`include "xdata_engdefs.vh"
                           
// FU defines              
`include "xmemdefs.vh"     
`include "xaludefs.vh"     
`include "xalulitedefs.vh" 
`include "xmuldefs.vh"     
`include "xmuladddefs.vh"  
`include "xbsdefs.vh"      
`include "xdivdefs.vh"     

module xtop (
                 input                                     clk,
                 input                                     rst,

                 // Controller interface
                 input                                     ctr_req,
                 input                                     ctr_rnw,
                 input      [`nVERSAT_W + `ADDR_W-1:0]     ctr_addr,
                 input      [`DATA_W-1:0]                  ctr_data_to_wr,
                 output reg [`DATA_W-1:0]                  ctr_data_to_rd,

                 // Configuration bus
                 input      [`CONF_BITS-1:0]               config_bus,

                 // DATABUS interface
                 input                                     databus_req,
                 input                                     databus_rnw,
                 input [`nVERSAT_W + `ADDR_W-1:0]          databus_addr,
                 input [`DATA_W-1:0]                       databus_data_in,
                 output reg [`DATA_W-1:0]                  databus_data_out
                 
                 );

   //This not work as verilator ignores the fact that different array elements
   //are usedi, that is why we have to use lint_off
   /* verilator lint_off UNOPTFLAT */ 
   wire [`DATA_BITS-1:0] databuses [`nVERSAT+1];
   wire [`DATA_W-1:0] versat_ctr_data_to_rd [`nVERSAT];
   wire [`DATA_W-1:0] versat_databus_data_out [`nVERSAT];
   /* verilator lint_on UNOPTFLAT */ 
   
   reg                run_req;
   reg [`nVERSAT-1:0] versat_ctr_req;
   reg [`nVERSAT-1:0] versat_databus_req;

   //Choose Versat to Write Conf
   always @ * begin
     integer j;
     versat_ctr_req = `nVERSAT'b0;
       for(j=0; j<`nVERSAT; j=j+1)
          if ( j[`nVERSAT_W-1:0] == ctr_addr[(`ADDR_W-1+`nVERSAT_W) -: `nVERSAT_W] )
            versat_ctr_req[j] = ctr_req;
   end 
   
   //Choose Versat to Write Data
   always @ * begin
     integer j;
     versat_databus_req = `nVERSAT'b0;
       for(j=0; j<`nVERSAT; j=j+1)
          if ( j[`nVERSAT_W-1:0] == databus_addr[(`ADDR_W-1+`nVERSAT_W) -: `nVERSAT_W] )
            versat_databus_req[j] = databus_req;
   end // block: addr_decoder
   
   //Choose Versat Read Conf
   always @ * begin
      integer j;
      ctr_data_to_rd = `DATA_W'd0;
      for (j = 0; j < `nVERSAT; j = j+1)
        if(j[`nVERSAT_W-1:0] == ctr_addr[(`nVERSAT_W + `ADDR_W-1) -: `nVERSAT_W])
          ctr_data_to_rd = versat_ctr_data_to_rd[j];
   end
   
   //Choose Versat Read Data
   always @ * begin
      integer j;
      databus_data_out = `DATA_W'd0;
      for (j = 0; j < `nVERSAT; j = j+1)
        if(j[`nVERSAT_W-1:0] == databus_addr[(`ADDR_W-1+`nVERSAT_W) -: `nVERSAT_W])
          databus_data_out = versat_databus_data_out[j];
   end
 
   //Compute Run Request
   always @ * begin
      run_req = 1'b0;
      if(ctr_addr[`ENG_ADDR_W-1:0] == `ENG_RUN_REG)
        run_req = ctr_req;
   end

   // Instantiate the Versats
   genvar i;
   generate
      for (i=0; i < `nVERSAT; i=i+1) begin : versat_array
        xversat versat (
                  .clk(clk),
                  .rst(rst),
                  
                  //Control Bus
                  .ctr_req(versat_ctr_req[i]),
                  .run_req(run_req),
                  .ctr_rnw(ctr_rnw),
                  .ctr_addr(ctr_addr[`ADDR_W-1:0]),
                  .ctr_data_to_wr(ctr_data_to_wr),
                  .ctr_data_to_rd(versat_ctr_data_to_rd[i]),
                  
                  //databus slave interface / DATA i/f
                  .databus_req(versat_databus_req[i]),
                  .databus_rnw(databus_rnw),
                  .databus_addr(databus_addr[`ADDR_W-1:0]),
                  .databus_data_in(databus_data_in),
                  .databus_data_out(versat_databus_data_out[i]),
                  
                  //signals used to connect 2 Versats      
                  .databus_data_prev(databuses[i][`DATA_BITS-1:0]),
                  .databus_data_next(databuses[i+1][`DATA_BITS-1:0])
                  );
      end
    endgenerate
    assign databuses[0] = databuses [`nVERSAT];     

endmodule

