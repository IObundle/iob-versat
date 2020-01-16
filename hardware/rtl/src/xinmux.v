`timescale 1ns / 1ps
`include "xdefs.vh"
`include "xdata_engdefs.vh"

module  xinmux (
    input [`N_W-1:0] 	         sel,
    input [`DATA_BITS-1:0] 	 data_bus,
    input [`DATA_BITS-1:0] 	 data_bus_prev,
    output reg [`DATA_W-1:0] data_out,
    output reg               enabled
    );

   always @ (*) begin : input_data_mux

      integer 		i;
      data_out = `DATA_W'd0;
      enabled  = 1'b1;
  // To select consts
      if (sel[`N_W-2:0] == `s0 )
        data_out =  data_bus[`DATA_BITS-1-`DATA_W*(`s0-1) -: `DATA_W];
      else if (sel[`N_W-2:0] == `s1)
        data_out =  data_bus[`DATA_BITS-1-`DATA_W*(`s1-1) -: `DATA_W ];
      else begin

	 // select memories
	 for (i=0; i < 2*`nMEM; i = i+1)
	   if ( sel[`N_W-2:0] == (`sMEM0A+i[`N_W-2:0]) )
	     data_out = (sel[`N_W-1]==0)? data_bus[`DATA_BITS-1-`DATA_W*(`sMEM0A-1+i) -: `DATA_W] : 
                                  data_bus_prev[`DATA_BITS-1-`DATA_W*(`sMEM0A-1+i) -: `DATA_W] ;

	 for (i=0; i < `nALU; i = i+1)
	   if ( sel[`N_W-2:0] == (`sALU0+i[`N_W-2:0]) )
	     data_out = (sel[`N_W-1]==0)? data_bus[`DATA_BITS-1-`DATA_W*(`sALU0-1+i-1) -: `DATA_W] :
                                  data_bus_prev[`DATA_BITS-1-`DATA_W*(`sALU0-1+i-1) -: `DATA_W];
	 
   for (i=0; i < `nALULITE; i = i+1)
	   if ( sel[`N_W-2:0] == (`sALULITE0+i[`N_W-2:0]) )
	     data_out = (sel[`N_W-1]==0)? data_bus[`DATA_BITS-1-`DATA_W*(`sALULITE0-1+i) -: `DATA_W]:
                                  data_bus_prev[`DATA_BITS-1-`DATA_W*(`sALULITE0-1+i) -: `DATA_W];
	 for (i=0; i < `nMUL; i = i+1)
	   if ( sel[`N_W-2:0] == (`sMUL0+i[`N_W-2:0]) )
	     data_out = (sel[`N_W-1]==0)? data_bus[`DATA_BITS-1-`DATA_W*(`sMUL0-1+i) -: `DATA_W]:
                                  data_bus_prev[`DATA_BITS-1-`DATA_W*(`sMUL0-1+i) -: `DATA_W];
	 
   for (i=0; i < `nMULADD; i = i+1)
	   if ( sel[`N_W-2:0] == (`sMULADD0+i[`N_W-2:0]) )
	     data_out = (sel[`N_W-1]==0)? data_bus[`DATA_BITS-1-`DATA_W*(`sMULADD0-1+i) -: `DATA_W]:
                                  data_bus_prev[`DATA_BITS-1-`DATA_W*(`sMULADD0-1+i) -: `DATA_W];

   for (i=0; i < `nBS; i = i+1)
	   if ( sel[`N_W-2:0] == (`sBS0+i[`N_W-2:0]) )
	     data_out = (sel[`N_W-1]==0)? data_bus[`DATA_BITS-1-`DATA_W*(`sBS0-1+i) -: `DATA_W]:
	                                data_bus_prev[`DATA_BITS-1-`DATA_W*(`sBS0-1+i) -: `DATA_W];

   if (sel[`N_W-2:0] == `sNONE || sel[`N_W-2:0] == `sADDR)	
	   enabled = 1'b0;     
      end
   end

endmodule
