`timescale 1ns / 1ps
`include "xversat.vh"
`include "xmemdefs.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"
`include "xconfdefs.vh"

`define iter	3
`define per	3
`define duty	3
`define shift 	(5-3)
`define incr	1
`define iter2	3
`define per2	3
`define shift2	(5-3)
`define incr2	1

module xdata_eng_tb;

   parameter clk_per = 10;
   parameter sMEM0A = 0;
   parameter sALU0 = sMEM0A + 2*`nMEM;
   parameter sALULITE0 = sALU0 + `nALU;
   parameter sMUL0 = sALULITE0 + `nALULITE;
   parameter sMULADD0 = sMUL0 + `nMUL;
   parameter sBS0 = sMULADD0 + `nMULADD;
   parameter DATA_W = 16;
 
   // Inputs
   reg 					clk;
   reg 					rst;
   
   //data/control interface
   reg 					valid;
   reg					we;
   reg [`nMEM_W+`MEM_ADDR_W:0]		addr;
   reg [DATA_W-1:0]			rdata;
   wire	signed [DATA_W-1:0]		wdata;

   //flow interface
   reg [`DATABUS_W-1:0] 		flow_in;
   wire [`DATABUS_W-1:0]		flow_out;

   //configuration bus
   reg [`CONF_BITS-1:0]			config_bus;
   
   integer i, j, k, l, err_cnt = 0;
   integer pixels[24:0], weights[8:0], bias;
   integer start_t, end_t;
   reg signed [DATA_W-1:0] res, res_exp;

   // Instantiate the Unit Under Test (UUT)
   xdata_eng # ( 
		.DATA_W(DATA_W)
   ) uut (
		.clk(clk), 
		.rst(rst),
		.valid(valid),
		.we(we),
		.addr(addr),
		.rdata(rdata),
		.wdata(wdata),
		.flow_in(flow_in),
		.flow_out(flow_out),
		.config_bus(config_bus)
	);

   initial begin
      
`ifdef DEBUG
      $dumpfile("xdata_eng.vcd");
      $dumpvars();   
`endif

      // Initialize Inputs
      clk = 0;
      rst = 1;
      valid = 0;
      we = 0;
      addr = 0;
      rdata = 0;
      flow_in = 0;
      config_bus = 0;
      
      // Wait 100 ns for global reset to finish
      #(clk_per*10) rst = 0;

      //write 5x5 feature map in mem0A
      for(i = 0; i < 25; i++) begin
        pixels[i] = $random%20;
        cpu_write(i, pixels[i]);        
      end
 
      //write 3x3 kernel and bias in mem1A
      for(i = 0; i < 10; i++) begin
        if(i < 9) begin 
          weights[i] = $random%2;
          cpu_write(i + 2**`MEM_ADDR_W, weights[i]);
        end else begin
          bias = $random%5;
          cpu_write(i + 2**`MEM_ADDR_W, bias);
        end
      end

     ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
     // 2D convolution with 2-loop addrgen
     ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

     //configure mem0A to read 3x3 block from feature map
     config_bus[`CONF_MEM0A_B -: `MEM_ADDR_W] = `iter; //iterations
     config_bus[`CONF_MEM0A_B - `MEM_ADDR_W -: `PERIOD_W] = `per; //period
     config_bus[`CONF_MEM0A_B - `MEM_ADDR_W-`PERIOD_W -: `PERIOD_W] = `duty; //duty
     config_bus[`CONF_MEM0A_B - 2*`MEM_ADDR_W-2*`PERIOD_W-`N_W -: `MEM_ADDR_W] = `shift; //shift
     config_bus[`CONF_MEM0A_B - 3*`MEM_ADDR_W-2*`PERIOD_W-`N_W -: `MEM_ADDR_W] = `incr; //incr 

     //configure mem1A to read kernel and bias
     config_bus[`CONF_MEM0A_B - 2*`MEMP_CONF_BITS -: `MEM_ADDR_W] = 1; //iterations
     config_bus[`CONF_MEM0A_B - 2*`MEMP_CONF_BITS - `MEM_ADDR_W -: `PERIOD_W] = 10; //period
     config_bus[`CONF_MEM0A_B - 2*`MEMP_CONF_BITS - `MEM_ADDR_W-`PERIOD_W -: `PERIOD_W] = 10; //duty
     config_bus[`CONF_MEM0A_B - 2*`MEMP_CONF_BITS - 3*`MEM_ADDR_W-2*`PERIOD_W-`N_W -: `MEM_ADDR_W] = 1; //incr 

     //configure muladd
     config_bus[`CONF_MULADD0_B -: `N_W] = sMEM0A; //sela
     config_bus[`CONF_MULADD0_B - `N_W -: `N_W] = sMEM0A+2; //selb
     config_bus[`CONF_MULADD0_B - 2*`N_W -: `MULADD_FNS_W] = `MULADD_MACC; //fns
     config_bus[`CONF_MULADD0_B - 2*`N_W - `MULADD_FNS_W -: `MEM_ADDR_W] = 1; //iterations
     config_bus[`CONF_MULADD0_B - 2*`N_W - `MULADD_FNS_W - `MEM_ADDR_W -: `PERIOD_W] = 9; //period
     config_bus[`CONF_MULADD0_B - 2*`N_W - `MULADD_FNS_W - `MEM_ADDR_W - `PERIOD_W -: `PERIOD_W] = `MEMP_LAT; //delay

     //configure ALULite to add bias to muladd result
     config_bus[`CONF_ALULITE0_B -: `N_W] = sMEM0A+2; //sela
     config_bus[`CONF_ALULITE0_B - `N_W -: `N_W] = sMULADD0; //selb
     config_bus[`CONF_ALULITE0_B - 2*`N_W - 1] = `ALULITE_ADD; //fns

     //config mem2A to store ALULite output
     config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS -: `MEM_ADDR_W] = 1; //iterations
     config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS - `MEM_ADDR_W -: `PERIOD_W] = 1; //period
     config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS - `MEM_ADDR_W-`PERIOD_W -: `PERIOD_W] = 1; //duty
     config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS - 4*`MEM_ADDR_W-2*`PERIOD_W-`N_W -: `PERIOD_W] = `MEMP_LAT + 8 + `MULADD_LAT + `ALULITE_LAT; //delay
     config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS - 3*`MEM_ADDR_W-2*`PERIOD_W-`N_W -: `MEM_ADDR_W] = 1; //incr 
     config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS - 4*`MEM_ADDR_W-3*`PERIOD_W-`N_W-1-1 -: 1] = 1; //wr_en
     config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS - `MEM_ADDR_W-2*`PERIOD_W -: `N_W] = sALULITE0; //sel

     //Loop for performing convolution
     start_t = $time;
     for(i = 0; i < `iter2; i++) begin
       for(j = 0; j < `per2; j++) begin
         
         //configure start values of memories
         config_bus[`CONF_MEM0A_B - `MEM_ADDR_W-2*`PERIOD_W-`N_W -: `MEM_ADDR_W] = i*5+j;
         config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS - `MEM_ADDR_W-2*`PERIOD_W-`N_W -: `MEM_ADDR_W] = i*3+j;

         //run configurations
         cpu_write(1<<(`nMEM_W+`MEM_ADDR_W), 1);

         //wait until config is done
         do
           cpu_read(1<<(`nMEM_W+`MEM_ADDR_W), res);
         while(res == 0);
       end
     end
     end_t = $time;

     //compare expected and actual results
     for(i = 0; i < `iter2; i++) begin
       for(j = 0; j < `per2; j++) begin
         res_exp = bias;
         for(k = 0; k < `iter; k++) begin
           for(l = 0; l < `per; l++) begin
             res_exp += pixels[i*5+j+k*5+l] * weights[k*3+l];
           end
         end
         cpu_read(2**(`MEM_ADDR_W+1) + 3*i + j, res);
         if(res != res_exp) err_cnt++;
       end
     end
     if(err_cnt == 0) $display("\n2D Convolution test with 2-loop addrgen passed in %0d ns", (end_t-start_t));
     else $display("\n2D Convolution test with 2-loop addrgen failed with %0d errors", err_cnt);
     err_cnt = 0;
     
     ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
     // 2D convolution with 4-loop addrgen
     ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

     //configure mem0A to read all 3x3 block from feature map
     config_bus[`CONF_MEM0A_B - `MEM_ADDR_W-2*`PERIOD_W-`N_W -: `MEM_ADDR_W] = 0; //start
     config_bus[`CONF_MEM0A_B-4*`MEM_ADDR_W-3*`PERIOD_W-`N_W-3 -: `MEM_ADDR_W] = `iter2;
     config_bus[`CONF_MEM0A_B-5*`MEM_ADDR_W-3*`PERIOD_W-`N_W-3 -: `PERIOD_W] = `per2;
     config_bus[`CONF_MEM0A_B-5*`MEM_ADDR_W-4*`PERIOD_W-`N_W-3 -: `MEM_ADDR_W] = `shift2;
     config_bus[`CONF_MEM0A_B-6*`MEM_ADDR_W-4*`PERIOD_W-`N_W-3 -: `MEM_ADDR_W] = `incr2;

     //configure mem1A to read kernel and bias
     config_bus[`CONF_MEM0A_B - 2*`MEMP_CONF_BITS -: `MEM_ADDR_W] = 9; //iterations
     config_bus[`CONF_MEM0A_B - 2*`MEMP_CONF_BITS - `MEM_ADDR_W -: `PERIOD_W] = 9; //period
     config_bus[`CONF_MEM0A_B - 2*`MEMP_CONF_BITS - `MEM_ADDR_W-`PERIOD_W -: `PERIOD_W] = 9; //duty
     config_bus[`CONF_MEM0A_B - 2*`MEMP_CONF_BITS - 2*`MEM_ADDR_W-2*`PERIOD_W-`N_W -: `MEM_ADDR_W] = -9; //shift

     //configure mem1B to read bias
     config_bus[`CONF_MEM0A_B - 3*`MEMP_CONF_BITS - `MEM_ADDR_W-2*`PERIOD_W-`N_W -: `MEM_ADDR_W] = 9; //start
     config_bus[`CONF_MEM0A_B - 3*`MEMP_CONF_BITS -: `MEM_ADDR_W] = 1; //iterations
     config_bus[`CONF_MEM0A_B - 3*`MEMP_CONF_BITS - `MEM_ADDR_W -: `PERIOD_W] = 1; //period
     config_bus[`CONF_MEM0A_B - 3*`MEMP_CONF_BITS - `MEM_ADDR_W-`PERIOD_W -: `PERIOD_W] = 1; //duty

     //configure muladd
     config_bus[`CONF_MULADD0_B - 2*`N_W - `MULADD_FNS_W -: `MEM_ADDR_W] = 9; //iterations

     //configure ALULite to add bias to muladd result
     config_bus[`CONF_ALULITE0_B -: `N_W] = sMEM0A+3; //sela

     //config mem2A to store ALULite output
     config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS - `MEM_ADDR_W-2*`PERIOD_W-`N_W -: `MEM_ADDR_W] = 50; //start
     config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS -: `MEM_ADDR_W] = 9; //iterations
     config_bus[`CONF_MEM0A_B - 4*`MEMP_CONF_BITS - `MEM_ADDR_W -: `PERIOD_W] = 9; //period

     //run configurations
     start_t = $time;
     cpu_write(1<<(`nMEM_W+`MEM_ADDR_W), 1);

     //wait until config is done
     do
       cpu_read(1<<(`nMEM_W+`MEM_ADDR_W), res);
     while(res == 0);
     end_t = $time;

     //compare expected and actual results
     for(i = 0; i < `iter2; i++) begin
       for(j = 0; j < `per2; j++) begin
         res_exp = bias;
         for(k = 0; k < `iter; k++) begin
           for(l = 0; l < `per; l++) begin
             res_exp += pixels[i*5+j+k*5+l] * weights[k*3+l];
           end
         end
	 cpu_read(2**(`MEM_ADDR_W+1) + 3*i + j + 50, res);
         if(res != res_exp) err_cnt++;
       end
     end

     //end simulation
     if(err_cnt == 0) $display("\n2D Convolution test with 4-loop addrgen passed in %0d ns\n", (end_t-start_t));
     else $display("\n2D Convolution test with 4-loop addrgen failed with %0d errors\n", err_cnt);
     $finish;
   end
	
   always 
     #(clk_per/2) clk = ~clk;

   //
   // CPU TASKS
   //

   task cpu_write;
      input [`nMEM_W+`MEM_ADDR_W:0] cpu_address;
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
      input [`nMEM_W+`MEM_ADDR_W:0] cpu_address;
      output [DATA_W-1:0] read_reg;
      addr = cpu_address;
      valid = 1;
      we = 0;
      #clk_per;
      read_reg = wdata;
      valid = 0;
      #clk_per;
   endtask

endmodule
