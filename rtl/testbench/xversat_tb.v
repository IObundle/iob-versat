`timescale 1ns / 1ps
`include "xversat.vh"
`include "xmemdefs.vh"
`include "versat-io.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"
`include "xconfdefs.vh"

`define iter    3
`define per     3
`define duty    3
`define shift   (5-3)
`define incr    1
`define iter2   3
`define per2    3
`define shift2  (5-3)
`define incr2   1

module xversat_tb;

   parameter			ADDR_W = 32;
   parameter			DATA_W = 32;

   //inputs
   reg 			   	clk;
   reg 			   	rst;

   //data/ctr interface
   reg 			   	    valid;
   reg [ADDR_W-1:0]  	addr;
   reg 			   	    we;
   reg [DATA_W-1:0]     rdata;
   wire				    ready;
   wire [DATA_W-1:0]    wdata;

   //parameters
   parameter 			clk_per     = 10;
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
   integer i, j, k, l, m, err_cnt = 0; 
   integer pixels[25*5-1:0], weights[9*5-1:0], bias;
   integer start_t, end_t, delay = 0, alu_sel;
   reg signed [DATA_W-1:0] res, res_exp;

   // Instantiate the Unit Under Test (UUT)
   xversat #(
	     .ADDR_W(ADDR_W),
	     .DATA_W(DATA_W)
            )
	uut (
	     .clk(clk),
	     .rst(rst),
	     .valid(valid),
	     .addr(addr),
	     .we(we),
	     .rdata(rdata),
	     .ready(ready),
	     .wdata(wdata)
	     );

   initial begin

`ifdef DEBUG
      $dumpfile("xversat.vcd");
      $dumpvars();
`endif
	
      //initialize inputs
      clk = 0;
      rst = 1;
      valid = 0;
      we = 0;
      addr = 0;
      rdata = 0;

      // Wait 100 ns for global reset to finish
      #(clk_per*10) rst = 0;

      //store data in versat mems
      for(j = 0; j < `nSTAGE; j++) begin

        //write 5x5 feature map in mem0
        for(i = 0; i < 25; i++) begin
          pixels[25*j+i] = $random%50;
          cpu_write((j<<(`CTR_ADDR_W-`nSTAGE_W)) + MEM0 + i, pixels[25*j+i]);
        end

        //write 3x3 kernel in mem1
        for(i = 0; i < 9; i++) begin
          weights[9*j+i] = $random%10;
          cpu_write((j<<(`CTR_ADDR_W-`nSTAGE_W)) + MEM1 + i, weights[9*j+i]);
        end

        //write bias after weights of VERSAT1
        if(j == 0) begin
          bias = $random%20;
          cpu_write(VERSAT_1 + MEM1 + 9, bias);
        end
      end

      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // 3D convolution with 2-loop addrgen
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

      alu_sel = sMEM0A+2;
      for(i = 0; i < `nSTAGE; i++) begin

        //configure mem0A to read 3x3 block from feature map
        config_mem(i<<(`CTR_ADDR_W-`nSTAGE_W), 0, `iter, `per, `duty, `shift, 1, delay, 0, 0);

        //configure mem1A to read kernel
        config_mem(i<<(`CTR_ADDR_W-`nSTAGE_W), 2, 1, 10, 10, 0, 1, delay, 0, 0);
      
        //configure muladd0
        config_muladd(i<<(`CTR_ADDR_W-`nSTAGE_W), sMEM0A, sMEM0A+2, `MULADD_MACC, 1, 9, `MEMP_LAT + delay, 0);

        //configure ALULite0 to add bias to muladd result
        config_alulite(i<<(`CTR_ADDR_W-`nSTAGE_W), alu_sel, sMULADD0, `ALULITE_ADD);

        //update variables
        if(i != `nSTAGE-1) delay += 2;
        if(i == 0) alu_sel = sALULITE0_p;
      end

      //config mem2A to store ALULite output
      config_mem((`nSTAGE-1)<<(`CTR_ADDR_W-`nSTAGE_W), 4, 1, 1, 1, 0, 1, `MEMP_LAT + 8 + `MULADD_LAT + `ALULITE_LAT + delay, sALULITE0, 1);

      ///////////////////////////////////////////////////////////////////////////////
      // Convolution loop
      ///////////////////////////////////////////////////////////////////////////////

      start_t = $time;
      for(i = 0; i < `iter2; i++) begin
        for(j = 0; j < `per2; j++) begin
          
          //configure start values of memories
	  for(k = 0; k < 5; k++) config_mem_start(k<<(`CTR_ADDR_W-`nSTAGE_W), 0, i*5+j);
          config_mem_start((`nSTAGE-1)<<(`CTR_ADDR_W-`nSTAGE_W), 4, i*3+j);

          //run configurations
          cpu_write(RUN_DONE, 1);

          //wait until config is done
          do
            cpu_read(RUN_DONE, res);
          while(res == 0);
        end
      end
      end_t = $time;

      ///////////////////////////////////////////////////////////////////////////////
      // DISPLAY RESULTS
      ///////////////////////////////////////////////////////////////////////////////
     
      for(i = 0; i < `iter2; i++) begin
        for(j = 0; j < `per2; j++) begin
          res_exp = bias;
          for(k = 0; k < `nSTAGE; k++) begin
            for(l = 0; l < `iter; l++) begin
              for(m = 0; m < `per; m++) begin
                res_exp += pixels[i*5+j+k*25+l*5+m] * weights[9*k+l*3+m];
              end
            end
          end
          cpu_read(((`nSTAGE-1)<<(`CTR_ADDR_W-`nSTAGE_W)) + MEM2 + i*3 + j, res);
          if(res != res_exp) err_cnt++;
        end
      end

      if(err_cnt == 0) $display("\n3D convolution test with 2-loop addrgen passed in %0d ns", (end_t-start_t));
      else $display("\n3D convolution test with 2-loop addrgen failed with %0d errors", err_cnt);
      err_cnt = 0;

      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // 3D convolution with 4-loop addrgen
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

      alu_sel = sMEM0A+3;
      delay = 0;

      //configure mem1B to read bias
      config_mem(VERSAT_1, 3, 1, 1, 1, 0, 0, 0, 0, 0);
      config_mem_start(VERSAT_1, 3, 9);

      for(i = 0; i < `nSTAGE; i++) begin

        //configure mem0A to read 3x3 block from feature map
	config_mem2(i<<(`CTR_ADDR_W-`nSTAGE_W), 0, `iter2, `per2, `shift2, `incr2);
        config_mem_start(i<<(`CTR_ADDR_W-`nSTAGE_W), 0, 0);

        //configure mem1A to read kernel
        config_mem(i<<(`CTR_ADDR_W-`nSTAGE_W), 2, 9, 9, 9, -9, 1, delay, 0, 0);
      
        //configure muladd0
        config_muladd(i<<(`CTR_ADDR_W-`nSTAGE_W), sMEM0A, sMEM0A+2, `MULADD_MACC, 9, 9, `MEMP_LAT + delay, 0);

        //configure ALULite0 to add bias to muladd result
        config_alulite(i<<(`CTR_ADDR_W-`nSTAGE_W), alu_sel, sMULADD0, `ALULITE_ADD);

        //update variables
        if(i != `nSTAGE-1) delay += 2;
        if(i == 0) alu_sel = sALULITE0_p;
      end

      //config mem2A to store ALULite output
      config_mem((`nSTAGE-1)<<(`CTR_ADDR_W-`nSTAGE_W), 4, 9, 9, 1, 0, 1, `MEMP_LAT + 8 + `MULADD_LAT + `ALULITE_LAT + delay, sALULITE0, 1);
      config_mem_start((`nSTAGE-1)<<(`CTR_ADDR_W-`nSTAGE_W), 4, 50);

      ///////////////////////////////////////////////////////////////////////////////
      // Convolution
      ///////////////////////////////////////////////////////////////////////////////

      //run configurations
      start_t = $time;
      cpu_write(RUN_DONE, 1);

      //wait until config is done
      do
        cpu_read(RUN_DONE, res);
      while(res == 0);
      end_t = $time;

      ///////////////////////////////////////////////////////////////////////////////
      // DISPLAY RESULTS
      ///////////////////////////////////////////////////////////////////////////////
     
      for(i = 0; i < `iter2; i++) begin
        for(j = 0; j < `per2; j++) begin
          res_exp = bias;
          for(k = 0; k < `nSTAGE; k++) begin
            for(l = 0; l < `iter; l++) begin
              for(m = 0; m < `per; m++) begin
                res_exp += pixels[i*5+j+k*25+l*5+m] * weights[9*k+l*3+m];
              end
            end
          end
          cpu_read(((`nSTAGE-1)<<(`CTR_ADDR_W-`nSTAGE_W)) + MEM2 + i*3 + j + 50, res);
          if(res != res_exp) err_cnt++;
        end
      end

      if(err_cnt == 0) $display("\n3D convolution test with 4-loop addrgen passed in %0d ns\n", (end_t-start_t));
      else $display("\n3D convolution test with 4-loop addrgen failed with %0d errors\n", err_cnt);

      ///////////////////////////////////////////////////////////////////////////////
      // CONF MEM and CONF CLEAR TEST
      ///////////////////////////////////////////////////////////////////////////////

      //clear conf_reg of VERSAT1
      cpu_write(VERSAT_1 + CONF_BASE + `CONF_CLEAR, 0);

`ifdef CONF_MEM_USE
      //store conf_reg of VERSAT2 in conf_mem (addr 0)
      cpu_write(VERSAT_2 + CONF_BASE + `CONF_MEM, 0);
`endif

      //global conf clear
      cpu_write(CONF_BASE + `GLOBAL_CONF_CLEAR, 0);

`ifdef CONF_MEM_USE
      //store conf_mem (addr 0) in conf_reg of VERSAT2
      cpu_read(VERSAT_2 + CONF_BASE + `CONF_MEM, res);
`endif
     
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
      input integer sel;
      input integer in_wr;
      integer base_addr;
      base_addr = stage + CONF_BASE + `CONF_MEM0A + mem_num*`MEMP_CONF_OFFSET;
      cpu_write(base_addr + `MEMP_CONF_ITER, iter);
      cpu_write(base_addr + `MEMP_CONF_PER, per);
      cpu_write(base_addr + `MEMP_CONF_DUTY, duty);
      cpu_write(base_addr + `MEMP_CONF_SHIFT, shift);
      cpu_write(base_addr + `MEMP_CONF_INCR, incr);
      cpu_write(base_addr + `MEMP_CONF_DELAY, delay);
      cpu_write(base_addr + `MEMP_CONF_SEL, sel);
      cpu_write(base_addr + `MEMP_CONF_IN_WR, in_wr);
   endtask   

   task config_mem2;
      input integer stage;
      input integer mem_num;
      input integer iter2;
      input integer per2;
      input integer shift2;
      input integer incr2;
      integer base_addr;
      base_addr = stage + CONF_BASE + `CONF_MEM0A + mem_num*`MEMP_CONF_OFFSET;
      cpu_write(base_addr + `MEMP_CONF_ITER2, iter2);
      cpu_write(base_addr + `MEMP_CONF_PER2, per2);
      cpu_write(base_addr + `MEMP_CONF_SHIFT2, shift2);
      cpu_write(base_addr + `MEMP_CONF_INCR2, incr2);
   endtask   

   task config_mem_start;
      input integer stage;
      input integer mem_num;
      input integer start;
      integer base_addr;
      base_addr = stage + CONF_BASE + `CONF_MEM0A + mem_num*`MEMP_CONF_OFFSET + `MEMP_CONF_START;
      cpu_write(base_addr, start);
   endtask

   task config_muladd;
      input integer stage;
      input integer selA;
      input integer selB;
      input integer fns;
      input integer iter;
      input integer per;
      input integer delay;
      input integer shift;
      integer base_addr;
      base_addr = stage + CONF_BASE + `CONF_MULADD0;
      cpu_write(base_addr + `MULADD_CONF_SELA, selA);
      cpu_write(base_addr + `MULADD_CONF_SELB, selB);
      cpu_write(base_addr + `MULADD_CONF_FNS, fns);
      cpu_write(base_addr + `MULADD_CONF_ITER, iter);
      cpu_write(base_addr + `MULADD_CONF_PER, per);
      cpu_write(base_addr + `MULADD_CONF_DELAY, delay);
      cpu_write(base_addr + `MULADD_CONF_SHIFT, shift);
   endtask

   task config_alulite;
      input integer stage;
      input integer selA;
      input integer selB;
      input integer fns;
      integer base_addr;
      base_addr = stage + CONF_BASE + `CONF_ALULITE0;
      cpu_write(base_addr + `ALULITE_CONF_SELA, selA);
      cpu_write(base_addr + `ALULITE_CONF_SELB, selB);
      cpu_write(base_addr + `ALULITE_CONF_FNS, fns);
   endtask

   task cpu_write;
      input [ADDR_W-1:0] cpu_address;
      input [DATA_W-1:0] cpu_data;
      addr = cpu_address;
      valid = 1;
      we = 1;
      rdata = cpu_data;
      #clk_per;
      we = 0;
      valid = 0;
      #clk_per;
   endtask

   task cpu_read;
      input [ADDR_W-1:0] cpu_address;
      output [DATA_W-1:0] read_reg;
      addr = cpu_address;
      valid = 1;
      we = 0;
      if(addr[`nMEM_W+`MEM_ADDR_W+1 -: 2] == 2'b0) #(clk_per*`MEMP_LAT); //wait 2 cycles if addressing mem
      else #clk_per;
      read_reg = wdata;
      valid = 0;
      #clk_per;
   endtask
 
endmodule
