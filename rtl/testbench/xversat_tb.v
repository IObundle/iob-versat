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
      for(i = 0; i < 25; i++) begin
        pixels[i] = $random%50;
        cpu_data_write(VERSAT_1 + MEM0 + i, pixels[i]);
      end

      //write 3x3 kernel and bias in mem1 for VERSAT1
      for(i = 0; i < 9; i++) begin
        weights[i] = $random%10;
        cpu_data_write(VERSAT_1 + MEM1 + i, weights[i]);
      end

      //write bias after weights of VERSAT1
      bias = $random%20;
      cpu_data_write(VERSAT_1 + MEM1 + 9, bias);

      //write 5x5 feature map in mem0 for VERSAT 2
      for(i = 0; i < 25; i++) begin
        pixels[25+i] = $random%50;
        cpu_data_write(VERSAT_2 + MEM0 + i, pixels[25+i]);
      end

      //write 3x3 kernel and bias in mem1 for VERSAT2
      for(i = 0; i < 9; i++) begin
        weights[9+i] = $random%10;
        cpu_data_write(VERSAT_2 + MEM1 + i, weights[9+i]);
      end

      //write 5x5 feature map in mem0 for VERSAT 3
      for(i = 0; i < 25; i++) begin
        pixels[50+i] = $random%50;
        cpu_data_write(VERSAT_3 + MEM0 + i, pixels[50+i]);
      end

      //write 3x3 kernel and bias in mem1 for VERSAT3
      for(i = 0; i < 9; i++) begin
        weights[18+i] = $random%10;
        cpu_data_write(VERSAT_3 + MEM1 + i, weights[18+i]);
      end

      //write 5x5 feature map in mem0 for VERSAT 4
      for(i = 0; i < 25; i++) begin
        pixels[75+i] = $random%50;
        cpu_data_write(VERSAT_4 + MEM0 + i, pixels[75+i]);
      end

      //write 3x3 kernel and bias in mem1 for VERSAT4
      for(i = 0; i < 9; i++) begin
        weights[27+i] = $random%10;
        cpu_data_write(VERSAT_4 + MEM1 + i, weights[27+i]);
      end

      //write 5x5 feature map in mem0 for VERSAT 5
      for(i = 0; i < 25; i++) begin
        pixels[100+i] = $random%50;
        cpu_data_write(VERSAT_5 + MEM0 + i, pixels[100+i]);
      end

      //write 3x3 kernel and bias in mem1 for VERSAT5
      for(i = 0; i < 9; i++) begin
        weights[36+i] = $random%10;
        cpu_data_write(VERSAT_5 + MEM1 + i, weights[36+i]);
      end

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

      //configure mem0A to read 3x3 block from feature map
      config_mem(VERSAT_1, 0, 3, 3, 3, 5-3, 1, 0, 0, 0, 0);

      //configure mem1A to read kernel
      config_mem(VERSAT_1, 2, 10, 1, 1, 0, 1, 0, 0, 0, 0);
      
      //configure mem1B to generate counter between 0 and 8 for muladd
      config_mem(VERSAT_1, 3, 8, 1, 1, 0, 1, `MEMP_LAT, 1, 0, 0);

      //configure muladd0
      config_muladd(VERSAT_1, sMEM0A, sMEM0A+2, sMEM0A+3, `MULADD_MUL_LOW_MACC, `MEMP_LAT);

      //configure ALULite0 to add bias to muladd result
      config_alulite(VERSAT_1, sMEM0A+2, sMULADD0, `ALULITE_ADD);

      ///////////////////////////////////////////////////////////////////////////////
      // VERSAT 2
      ///////////////////////////////////////////////////////////////////////////////

      //configure mem0A to read 3x3 block from feature map
      config_mem(VERSAT_2, 0, 3, 3, 3, 5-3, 1, 2, 0, 0, 0);

      //configure mem1A to read kernel
      config_mem(VERSAT_2, 2, 10, 1, 1, 0, 1, 2, 0, 0, 0);
      
      //configure mem1B to generate counter between 0 and 8 for muladd
      config_mem(VERSAT_2, 3, 8, 1, 1, 0, 1, 2, 1, 0, 0);

      //configure muladd0
      config_muladd(VERSAT_2, sMEM0A, sMEM0A+2, sMEM0A+3, `MULADD_MUL_LOW_MACC, `MEMP_LAT + 2);

      //configure ALULite0 to add bias to muladd result
      config_alulite(VERSAT_2, sALULITE0_p, sMULADD0, `ALULITE_ADD);

      ///////////////////////////////////////////////////////////////////////////////
      // VERSAT 3
      ///////////////////////////////////////////////////////////////////////////////

      //configure mem0A to read 3x3 block from feature map
      config_mem(VERSAT_3, 0, 3, 3, 3, 5-3, 1, 4, 0, 0, 0);

      //configure mem1A to read kernel
      config_mem(VERSAT_3, 2, 10, 1, 1, 0, 1, 4, 0, 0, 0);
      
      //configure mem1B to generate counter between 0 and 8 for muladd
      config_mem(VERSAT_3, 3, 8, 1, 1, 0, 1, `MEMP_LAT + 4, 1, 0, 0);

      //configure muladd0
      config_muladd(VERSAT_3, sMEM0A, sMEM0A+2, sMEM0A+3, `MULADD_MUL_LOW_MACC, `MEMP_LAT + 4);

      //configure ALULite0 to add bias to muladd result
      config_alulite(VERSAT_3, sALULITE0_p, sMULADD0, `ALULITE_ADD);

      ///////////////////////////////////////////////////////////////////////////////
      // VERSAT 4
      ///////////////////////////////////////////////////////////////////////////////

      //configure mem0A to read 3x3 block from feature map
      config_mem(VERSAT_4, 0, 3, 3, 3, 5-3, 1, 6, 0, 0, 0);

      //configure mem1A to read kernel
      config_mem(VERSAT_4, 2, 10, 1, 1, 0, 1, 6, 0, 0, 0);
      
      //configure mem1B to generate counter between 0 and 8 for muladd
      config_mem(VERSAT_4, 3, 8, 1, 1, 0, 1, `MEMP_LAT + 6, 1, 0, 0);

      //configure muladd0
      config_muladd(VERSAT_4, sMEM0A, sMEM0A+2, sMEM0A+3, `MULADD_MUL_LOW_MACC, `MEMP_LAT + 6);

      //configure ALULite0 to add bias to muladd result
      config_alulite(VERSAT_4, sALULITE0_p, sMULADD0, `ALULITE_ADD);

      ///////////////////////////////////////////////////////////////////////////////
      // VERSAT 5
      ///////////////////////////////////////////////////////////////////////////////

      //configure mem0A to read 3x3 block from feature map
      config_mem(VERSAT_5, 0, 3, 3, 3, 5-3, 1, 8, 0, 0, 0);

      //configure mem1A to read kernel
      config_mem(VERSAT_5, 2, 10, 1, 1, 0, 1, 8, 0, 0, 0);
      
      //configure mem1B to generate counter between 0 and 8 for muladd
      config_mem(VERSAT_5, 3, 8, 1, 1, 0, 1, `MEMP_LAT + 8, 1, 0, 0);

      //configure muladd0
      config_muladd(VERSAT_5, sMEM0A, sMEM0A+2, sMEM0A+3, `MULADD_MUL_LOW_MACC, `MEMP_LAT + 8);

      //configure mem1A to read kernel
      config_alulite(VERSAT_5, sALULITE0_p, sMULADD0, `ALULITE_ADD);

      //config mem2A to store ALULite output
      config_mem(VERSAT_5, 4, 1, 1, 1, 0, 1, `MEMP_LAT + 8 + `MULADD_LAT + `ALULITE_LAT + 8, 0, sALULITE0, 1);

      ///////////////////////////////////////////////////////////////////////////////
      // Convolution loop
      ///////////////////////////////////////////////////////////////////////////////

      $display("\nActual convolution result");
      for(i = 0; i < 3; i++) begin
        for(j = 0; j < 3; j++) begin
          
          //configure start values of memories
          config_mem_start(VERSAT_1, 0, i*5+j);
          config_mem_start(VERSAT_2, 0, i*5+j);
          config_mem_start(VERSAT_3, 0, i*5+j);
          config_mem_start(VERSAT_4, 0, i*5+j);
          config_mem_start(VERSAT_5, 0, i*5+j);
          config_mem_start(VERSAT_5, 4, i*3+j);

          //run configurations
          cpu_ctr_write(RUN_DONE, 1);

          //wait until config is done
          do
            cpu_ctr_read(RUN_DONE, res);
          while(res == 0);
        end
      end

      ///////////////////////////////////////////////////////////////////////////////
      // DISPLAY RESULTS
      ///////////////////////////////////////////////////////////////////////////////
     
      for(i = 0; i < 3; i++) begin
        for(j = 0; j < 3; j++) begin
          cpu_data_read(VERSAT_5 + MEM2 + i*3 + j, res);
          $write("%d ", res);
        end
        $write("\n"); 
      end

      //end simulation
      $finish;

   end // initial begin

   // Clock generation
   always
     #(clk_per/2) clk = ~clk;

   //
   // CPU TASKS
   //

   task config_mem;
      input integer stage;
      input integer mem_num;
      input integer iter;
      input integer per;
      input integer duty;
      input integer shift;
      input integer incr;
      input integer delay;
      input integer addr_out_en;
      input integer sel;
      input integer in_wr;
      integer base_addr;
      base_addr = stage + CONF_BASE + `CONF_MEM0A + mem_num*`MEMP_CONF_OFFSET;
      cpu_ctr_write(base_addr + `MEMP_CONF_ITER, iter);
      cpu_ctr_write(base_addr + `MEMP_CONF_PER, per);
      cpu_ctr_write(base_addr + `MEMP_CONF_DUTY, duty);
      cpu_ctr_write(base_addr + `MEMP_CONF_SHIFT, shift);
      cpu_ctr_write(base_addr + `MEMP_CONF_INCR, incr);
      cpu_ctr_write(base_addr + `MEMP_CONF_DELAY, delay);
      cpu_ctr_write(base_addr + `MEMP_CONF_ADDR_OUT_EN, addr_out_en);
      cpu_ctr_write(base_addr + `MEMP_CONF_SEL, sel);
      cpu_ctr_write(base_addr + `MEMP_CONF_IN_WR, in_wr);
   endtask   

   task config_mem_start;
      input integer stage;
      input integer mem_num;
      input integer start;
      integer base_addr;
      base_addr = stage + CONF_BASE + `CONF_MEM0A + mem_num*`MEMP_CONF_OFFSET + `MEMP_CONF_START;
      cpu_ctr_write(base_addr, start);
   endtask

   task config_muladd;
      input integer stage;
      input integer selA;
      input integer selB;
      input integer selO;
      input integer fns;
      input integer delay;
      integer base_addr;
      base_addr = stage + CONF_BASE + `CONF_MULADD0;
      cpu_ctr_write(base_addr + `MULADD_CONF_SELA, selA);
      cpu_ctr_write(base_addr + `MULADD_CONF_SELB, selB);
      cpu_ctr_write(base_addr + `MULADD_CONF_SELO, selO);
      cpu_ctr_write(base_addr + `MULADD_CONF_FNS, fns);
      cpu_ctr_write(base_addr + `MULADD_CONF_DELAY, delay);
   endtask

   task config_alulite;
      input integer stage;
      input integer selA;
      input integer selB;
      input integer fns;
      integer base_addr;
      base_addr = stage + CONF_BASE + `CONF_ALULITE0;
      cpu_ctr_write(base_addr + `ALULITE_CONF_SELA, selA);
      cpu_ctr_write(base_addr + `ALULITE_CONF_SELB, selB);
      cpu_ctr_write(base_addr + `ALULITE_CONF_FNS, fns);
   endtask

   task cpu_data_write;
      input [`CTR_ADDR_W-1:0] cpu_address;
      input [`DATA_W-1:0] cpu_data;
      data_addr = cpu_address;
      data_valid = 1;
      data_we = 1;
      data_data_in = cpu_data;
      #clk_per;
      data_we = 0;
      data_valid = 0;
      #clk_per;
   endtask

   task cpu_data_read;
      input [`CTR_ADDR_W-1:0] cpu_address;
      output [`DATA_W-1:0] read_reg;
      data_addr = cpu_address;
      data_valid = 1;
      data_we = 0;
      #clk_per;
      read_reg = data_data_out;
      data_valid = 0;
      #clk_per;
   endtask

   task cpu_ctr_write;
      input [`CTR_ADDR_W-1:0] cpu_address;
      input [`DATA_W-1:0] cpu_data;
      ctr_addr = cpu_address;
      ctr_valid = 1;
      ctr_we = 1;
      ctr_data_in = cpu_data;
      #clk_per;
      ctr_we = 0;
      ctr_valid = 0;
      #clk_per;
   endtask

   task cpu_ctr_read;
      input [`CTR_ADDR_W-1:0] cpu_address;
      output [`DATA_W-1:0] read_reg;
      ctr_addr = cpu_address;
      ctr_valid = 1;
      ctr_we = 0;
      #clk_per;
      read_reg = ctr_data_out;
      ctr_valid = 0;
      #clk_per;
   endtask
 
endmodule
