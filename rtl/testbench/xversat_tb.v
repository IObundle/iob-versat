`timescale 1ns / 1ps
`include "xversat.vh"
`include "xmemdefs.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"
`include "xconfdefs.vh"

module xversat_tb;

   //inputs
   reg 			   	clk;
   reg 			   	rst;

   //ctr interface
   reg 			   	ctr_valid;
   reg [`CTR_ADDR_W-1:0]  	ctr_addr;
   reg 			   	ctr_we;
   reg [`DATA_W-1:0]       	ctr_data_in;
   wire [`DATA_W-1:0]     	ctr_data_out;

   //data interface
   reg 			   	data_valid;
   reg 			   	data_we;
   reg [`CTR_ADDR_W-1:0] 	data_addr;
   reg [`DATA_W-1:0] 	   	data_data_in;
   wire signed [`DATA_W-1:0]   	data_data_out;

   //parameters
   parameter 			clk_per     = 20;
   parameter			VERSAT_1    = (0<<(`CTR_ADDR_W-`nSTAGE_W));
   parameter			VERSAT_2    = (1<<(`CTR_ADDR_W-`nSTAGE_W));
   parameter			VERSAT_3    = (2<<(`CTR_ADDR_W-`nSTAGE_W));
   parameter			VERSAT_4    = (3<<(`CTR_ADDR_W-`nSTAGE_W));
   parameter			VERSAT_5    = (4<<(`CTR_ADDR_W-`nSTAGE_W));
   parameter			MEM0        = (0<<`MEM_ADDR_W);
   parameter			MEM1        = (1<<`MEM_ADDR_W);
   parameter			MEM2        = (2<<`MEM_ADDR_W);
   parameter			RUN_DONE    = (1<<(`nMEM_W+`MEM_ADDR_W));
   parameter			CONF_BASE   = (1<<(`nMEM_W+`MEM_ADDR_W+1));
   parameter 			sMEM0A      = 0;
   parameter 			sALU0       = sMEM0A + 2*`nMEM;
   parameter 			sALULITE0   = sALU0 + `nALU;
   parameter 			sMUL0       = sALULITE0 + `nALULITE;
   parameter 			sMULADD0    = sMUL0 + `nMUL;
   parameter 			sBS0        = sMULADD0 + `nMULADD;
   parameter 			sMEM0A_p    = 0 + (1<<(`N_W-1));
   parameter 			sALU0_p     = sMEM0A + 2*`nMEM + (1<<(`N_W-1));
   parameter 			sALULITE0_p = sALU0 + `nALU + (1<<(`N_W-1));
   parameter 			sMUL0_p     = sALULITE0 + `nALULITE + (1<<(`N_W-1));
   parameter 			sMULADD0_p  = sMUL0 + `nMUL + (1<<(`N_W-1));
   parameter 			sBS0_p      = sMULADD0 + `nMULADD + (1<<(`N_W-1));

   //integer variables
   integer i, j, k, l, m; 
   integer pixels[25*5-1:0], weights[9*5-1:0], bias, res;

   // Instantiate the Unit Under Test (UUT)
   xversat uut (
	     .clk(clk),
	     .rst(rst),
	     .ctr_valid(ctr_valid),
	     .ctr_addr(ctr_addr),
	     .ctr_we(ctr_we),
	     .ctr_data_in(ctr_data_in),
	     .ctr_data_out(ctr_data_out),
	     .data_valid(data_valid),
	     .data_we(data_we),
	     .data_addr(data_addr),
	     .data_data_in(data_data_in),
	     .data_data_out(data_data_out)
	     );

   initial begin

`ifdef DEBUG
      $dumpfile("xversat.vcd");
      $dumpvars();
`endif
	
      $display("\nTesting one 3D convolution with five 5x5 input FMs and five 3x3 kernels");

      //initialize inputs
      clk = 0;
      rst = 1;
      ctr_valid = 0;
      ctr_we = 0;
      ctr_addr = 0;
      ctr_data_in = 0;
      data_valid = 0;
      data_we = 0;
      data_addr = 0;
      data_data_in = 0;

      // Wait 100 ns for global reset to finish
      #(clk_per*5) rst = 0;

      //write 5x5 feature map in mem0 for VERSAT 1
      data_valid = 1;
      data_we = 1;
      for(i = 0; i < 25; i++) begin
        data_addr = VERSAT_1 + MEM0 + i;
        data_data_in = $random%50;
        pixels[i] = data_data_in;
        #clk_per;
      end

      //write 3x3 kernel and bias in mem1 for VERSAT1
      for(i = 0; i < 9; i++) begin
        data_addr = VERSAT_1 + MEM1 + i;
        data_data_in = $random%10;
        weights[i] = data_data_in;
        #clk_per;
      end

      //write bias after weights of VERSAT1
      data_addr++;
      data_data_in = $random%20;
      bias = data_data_in;
      #clk_per;

      //write 5x5 feature map in mem0 for VERSAT 2
      for(i = 0; i < 25; i++) begin
        data_addr = VERSAT_2 + MEM0 + i;
        data_data_in = $random%50;
        pixels[25+i] = data_data_in;
        #clk_per;
      end

      //write 3x3 kernel and bias in mem1 for VERSAT2
      for(i = 0; i < 9; i++) begin
        data_addr = VERSAT_2 + MEM1 + i;
        data_data_in = $random%10;
        weights[9+i] = data_data_in;
        #clk_per;
      end

      //write 5x5 feature map in mem0 for VERSAT 3
      for(i = 0; i < 25; i++) begin
        data_addr = VERSAT_3 + MEM0 + i;
        data_data_in = $random%50;
        pixels[50+i] = data_data_in;
        #clk_per;
      end

      //write 3x3 kernel and bias in mem1 for VERSAT3
      for(i = 0; i < 9; i++) begin
        data_addr = VERSAT_3 + MEM1 + i;
        data_data_in = $random%10;
        weights[18+i] = data_data_in;
        #clk_per;
      end

      //write 5x5 feature map in mem0 for VERSAT 4
      for(i = 0; i < 25; i++) begin
        data_addr = VERSAT_4 + MEM0 + i;
        data_data_in = $random%50;
        pixels[75+i] = data_data_in;
        #clk_per;
      end

      //write 3x3 kernel and bias in mem1 for VERSAT4
      for(i = 0; i < 9; i++) begin
        data_addr = VERSAT_4 + MEM1 + i;
        data_data_in = $random%10;
        weights[27+i] = data_data_in;
        #clk_per;
      end

      //write 5x5 feature map in mem0 for VERSAT 5
      for(i = 0; i < 25; i++) begin
        data_addr = VERSAT_5 + MEM0 + i;
        data_data_in = $random%50;
        pixels[100+i] = data_data_in;
        #clk_per;
      end

      //write 3x3 kernel and bias in mem1 for VERSAT5
      for(i = 0; i < 9; i++) begin
        data_addr = VERSAT_5 + MEM1 + i;
        data_data_in = $random%10;
        weights[36+i] = data_data_in;
        #clk_per;
      end

      //stop writing
      data_valid = 0;
      data_we = 0;

      //expected result of 3D convolution
      $display("\nExpected result of 3D convolution"); 
      for(i = 0; i < 3; i++) begin
        for(j = 0; j < 3; j++) begin
          res = bias;
          for(k = 0; k < 5; k++) begin
            for(l = 0; l < 3; l++) begin
              for(m = 0; m < 3; m++) begin
                res += pixels[i*5+j+k*25+l*5+m] * weights[9*k+l*3+m];
              end
            end
          end
          $write("%d", res);
        end
        $write("\n");
      end

      ///////////////////////////////////////////////////////////////////////////////
      // VERSAT 1
      ///////////////////////////////////////////////////////////////////////////////

      ctr_valid = 1;
      ctr_we = 1;

      //configure mem0A to read 3x3 block from feature map
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 3; //iterations
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 3; //period
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 3; //duty
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_SHIFT;
      ctr_data_in = 5-3; //shift
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;

      //configure mem1A to read kernel
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 10; //iterations
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      
      //configure mem1B to generate counter between 0 and 8 for muladd
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 8; //iterations
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = `MEMP_LAT; //delay
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_ADDR_OUT_EN;
      ctr_data_in = 1; //sADDR
      #clk_per;

      //configure muladd0
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELA;
      ctr_data_in = sMEM0A; //selA
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELB;
      ctr_data_in = sMEM0A+2; //selB
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELO;
      ctr_data_in = sMEM0A+3; //selO
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_FNS;
      ctr_data_in = `MULADD_MUL_LOW_MACC; //fns
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_DELAY;
      ctr_data_in = `MEMP_LAT; //delay
      #clk_per;

      //configure ALULite0 to add bias to muladd result
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_SELA;
      ctr_data_in = sMEM0A+2; //selA
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_SELB;
      ctr_data_in = sMULADD0; //selB
      #clk_per;
      ctr_addr = VERSAT_1 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_FNS;
      ctr_data_in = `ALULITE_ADD; //fns
      #clk_per;

      ///////////////////////////////////////////////////////////////////////////////
      // VERSAT 2
      ///////////////////////////////////////////////////////////////////////////////

      //configure mem0A to read 3x3 block from feature map
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 3; //iterations
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 3; //period
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 3; //duty
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_SHIFT;
      ctr_data_in = 5-3; //shift
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = 2; //delay
      #clk_per;

      //configure mem1A to read kernel
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 10; //iterations
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = 2; //delay
      #clk_per;
      
      //configure mem1B to generate counter between 0 and 8 for muladd
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 8; //iterations
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = `MEMP_LAT + 2; //delay
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_ADDR_OUT_EN;
      ctr_data_in = 1; //sADDR
      #clk_per;

      //configure muladd0
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELA;
      ctr_data_in = sMEM0A; //selA
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELB;
      ctr_data_in = sMEM0A+2; //selB
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELO;
      ctr_data_in = sMEM0A+3; //selO
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_FNS;
      ctr_data_in = `MULADD_MUL_LOW_MACC; //fns
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_DELAY;
      ctr_data_in = `MEMP_LAT + 2; //delay
      #clk_per;

      //configure ALULite0 to add bias to muladd result
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_SELA;
      ctr_data_in = sALULITE0_p; //selA
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_SELB;
      ctr_data_in = sMULADD0; //selB
      #clk_per;
      ctr_addr = VERSAT_2 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_FNS;
      ctr_data_in = `ALULITE_ADD; //fns
      #clk_per;

      ///////////////////////////////////////////////////////////////////////////////
      // VERSAT 3
      ///////////////////////////////////////////////////////////////////////////////

      //configure mem0A to read 3x3 block from feature map
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 3; //iterations
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 3; //period
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 3; //duty
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_SHIFT;
      ctr_data_in = 5-3; //shift
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = 4; //delay
      #clk_per;

      //configure mem1A to read kernel
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 10; //iterations
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = 4; //delay
      #clk_per;
      
      //configure mem1B to generate counter between 0 and 8 for muladd
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 8; //iterations
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = `MEMP_LAT + 4; //delay
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_ADDR_OUT_EN;
      ctr_data_in = 1; //sADDR
      #clk_per;

      //configure muladd0
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELA;
      ctr_data_in = sMEM0A; //selA
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELB;
      ctr_data_in = sMEM0A+2; //selB
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELO;
      ctr_data_in = sMEM0A+3; //selO
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_FNS;
      ctr_data_in = `MULADD_MUL_LOW_MACC; //fns
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_DELAY;
      ctr_data_in = `MEMP_LAT + 4; //delay
      #clk_per;

      //configure ALULite0 to add bias to muladd result
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_SELA;
      ctr_data_in = sALULITE0_p; //selA
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_SELB;
      ctr_data_in = sMULADD0; //selB
      #clk_per;
      ctr_addr = VERSAT_3 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_FNS;
      ctr_data_in = `ALULITE_ADD; //fns
      #clk_per;

      ///////////////////////////////////////////////////////////////////////////////
      // VERSAT 4
      ///////////////////////////////////////////////////////////////////////////////

      //configure mem0A to read 3x3 block from feature map
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 3; //iterations
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 3; //period
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 3; //duty
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_SHIFT;
      ctr_data_in = 5-3; //shift
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = 6; //delay
      #clk_per;

      //configure mem1A to read kernel
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 10; //iterations
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = 6; //delay
      #clk_per;
      
      //configure mem1B to generate counter between 0 and 8 for muladd
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 8; //iterations
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = `MEMP_LAT + 6; //delay
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_ADDR_OUT_EN;
      ctr_data_in = 1; //sADDR
      #clk_per;

      //configure muladd0
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELA;
      ctr_data_in = sMEM0A; //selA
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELB;
      ctr_data_in = sMEM0A+2; //selB
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELO;
      ctr_data_in = sMEM0A+3; //selO
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_FNS;
      ctr_data_in = `MULADD_MUL_LOW_MACC; //fns
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_DELAY;
      ctr_data_in = `MEMP_LAT + 6; //delay
      #clk_per;

      //configure ALULite0 to add bias to muladd result
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_SELA;
      ctr_data_in = sALULITE0_p; //selA
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_SELB;
      ctr_data_in = sMULADD0; //selB
      #clk_per;
      ctr_addr = VERSAT_4 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_FNS;
      ctr_data_in = `ALULITE_ADD; //fns
      #clk_per;

      ///////////////////////////////////////////////////////////////////////////////
      // VERSAT 5
      ///////////////////////////////////////////////////////////////////////////////

      //configure mem0A to read 3x3 block from feature map
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 3; //iterations
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 3; //period
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 3; //duty
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_SHIFT;
      ctr_data_in = 5-3; //shift
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = 8; //delay
      #clk_per;

      //configure mem1A to read kernel
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 10; //iterations
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 2*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = 8; //delay
      #clk_per;
      
      //configure mem1B to generate counter between 0 and 8 for muladd
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 8; //iterations
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = `MEMP_LAT + 8; //delay
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 3*`MEMP_CONF_OFFSET + `MEMP_CONF_ADDR_OUT_EN;
      ctr_data_in = 1; //sADDR
      #clk_per;

      //configure muladd0
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELA;
      ctr_data_in = sMEM0A; //selA
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELB;
      ctr_data_in = sMEM0A+2; //selB
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_SELO;
      ctr_data_in = sMEM0A+3; //selO
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_FNS;
      ctr_data_in = `MULADD_MUL_LOW_MACC; //fns
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MULADD0 + `MULADD_CONF_DELAY;
      ctr_data_in = `MEMP_LAT + 8; //delay
      #clk_per;

      //configure ALULite0 to add bias to muladd result
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_SELA;
      ctr_data_in = sALULITE0_p; //selA
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_SELB;
      ctr_data_in = sMULADD0; //selB
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_ALULITE0 + `ALULITE_CONF_FNS;
      ctr_data_in = `ALULITE_ADD; //fns
      #clk_per;

      //config mem2A to store ALULite output
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 4*`MEMP_CONF_OFFSET + `MEMP_CONF_ITER;
      ctr_data_in = 1; //iterations
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 4*`MEMP_CONF_OFFSET + `MEMP_CONF_PER;
      ctr_data_in = 1; //period
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 4*`MEMP_CONF_OFFSET + `MEMP_CONF_DUTY;
      ctr_data_in = 1; //duty
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 4*`MEMP_CONF_OFFSET + `MEMP_CONF_INCR;
      ctr_data_in = 1; //incr
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 4*`MEMP_CONF_OFFSET + `MEMP_CONF_DELAY;
      ctr_data_in = `MEMP_LAT + 8 + `MULADD_LAT + `ALULITE_LAT + 8; //delay
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 4*`MEMP_CONF_OFFSET + `MEMP_CONF_SEL;
      ctr_data_in = sALULITE0; //sel
      #clk_per;
      ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 4*`MEMP_CONF_OFFSET + `MEMP_CONF_IN_WR;
      ctr_data_in = 1; //wr_en
      #clk_per;

      ///////////////////////////////////////////////////////////////////////////////
      // Convolution loop
      ///////////////////////////////////////////////////////////////////////////////

      $display("\nActual convolution result");
      for(i = 0; i < 3; i++) begin
        for(j = 0; j < 3; j++) begin
          
          //configure start values of memories
          ctr_we = 1;
          ctr_addr = VERSAT_1 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_START;
          ctr_data_in = i*5+j; //start
          #clk_per;
          ctr_addr = VERSAT_2 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_START;
          #clk_per;
          ctr_addr = VERSAT_3 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_START;
          #clk_per;
          ctr_addr = VERSAT_4 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_START;
          #clk_per;
          ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 0*`MEMP_CONF_OFFSET + `MEMP_CONF_START;
          #clk_per;
          ctr_addr = VERSAT_5 + CONF_BASE + `CONF_MEM0A + 4*`MEMP_CONF_OFFSET + `MEMP_CONF_START;
          ctr_data_in = i*3+j; //start
          #clk_per;

          //run configurations
          ctr_addr = RUN_DONE;
          ctr_data_in[0] = 1;
          #clk_per;

          //wait until config is done
          ctr_we = 0;
          ctr_data_in = 0;
          do begin
           #clk_per;
          end while(ctr_data_out == 0);
        end
      end

      ///////////////////////////////////////////////////////////////////////////////
      // DISPLAY RESULTS
      ///////////////////////////////////////////////////////////////////////////////
     
      //stop controlling
      ctr_valid = 0;
      data_valid = 1;

      for(i = 0; i < 3; i++) begin
        for(j = 0; j < 3; j++) begin
          data_addr = VERSAT_5 + MEM2 + i*3 + j;
          #clk_per $write("%d ", data_data_out);
        end
        $write("\n"); 
      end

      //end simulation
      data_valid = 0;
      $finish;

   end // initial begin

   // Clock generation
   always
     #(clk_per/2) clk = ~clk;
 
endmodule
