`timescale 1ns / 1ps

`include "xmemdefs.vh"

`define iter	3
`define per  	3
`define duty 	3
`define shift 	(5-3)
`define incr	1
`define iter2	3
`define per2	3
`define shift2	(5-3)
`define incr2	1

module xmem_tb;

   //parameters 
   parameter clk_per = 20;
   parameter DATA_W = 16;
   integer i, j, k, l, err_cnt = 0;
   integer pixels[25-1:0];
   reg signed [DATA_W-1:0] aux_val;
   
   //control 
   reg 				clk;
   reg 				rst;
   reg				run;
   wire 			doneA, doneB;

   //mem interface
   reg 				we;
   reg [`MEM_ADDR_W-1:0] 	addr;
   reg [DATA_W-1:0] 		rdata;
   reg 				valid;
   
   //input / output data
   reg [2*`DATABUS_W-1:0] 	flow_in;
   wire [2*DATA_W-1:0]   	flow_out;
   
   //configurations
   reg [2*`MEMP_CONF_BITS-1:0] 	config_bits;
   
   // Instantiate the Unit Under Test (UUT)
   xmem # ( 
	     .DATA_W(DATA_W)
   ) uut (
	     //control 
	     .clk(clk),
	     .rst(rst),
   	     .run(run),
             .doneA(doneA),
	     .doneB(doneB),

	     //mem interface
 	     .we(we),
	     .addr(addr),
	     .rdata(rdata),
	     .valid(valid),

	     //input / output data
	     .flow_in(flow_in),
	     .flow_out(flow_out),
	     
	     //configurations
	     .config_bits(config_bits)
	     
	     );

   initial begin
      
`ifdef DEBUG
      $dumpfile("xmem.vcd");
      $dumpvars();
`endif
      
      //inputs
      clk = 0;
      rst = 1;
      run = 0;

      we = 0;
      addr = 0;
      rdata = 0;
      valid = 0;
     
      flow_in = 0;
      config_bits = 0;
      
      //Global reset (100ns)
      #(clk_per*5) rst = 0;

      //Testing mem interface
      $display("\nTesting mem interface...");
      
      //Write values to memory
      for(i = 0; i < 25; i++) begin
        pixels[i] = $random%50;
        cpu_write(i, pixels[i]);
      end

      //Read values back from memory
      for(i = 0; i < 25; i++) begin
        cpu_read(i, aux_val);
	if(aux_val != pixels[i]) err_cnt++;
      end

      //Check mem interface
      if(err_cnt == 0) $display("Mem interface passed test");
      else $display("Mem interface failed test with %d errors", err_cnt);
      err_cnt = 0;

      //Testing address generator and configuration bus
      $display("\nTesting address generator and configuration bus...");

      //configurations
      //simulate reading all 3x3 blocks of 5x5 feature map
      config_bits[2*`MEMP_CONF_BITS-1 -: `MEM_ADDR_W] = `iter; //iterations
      config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-1 -: `PERIOD_W] = `per; //period
      config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-`PERIOD_W-1 -: `PERIOD_W] = `duty; //duty
      config_bits[2*`MEMP_CONF_BITS-2*`MEM_ADDR_W-2*`PERIOD_W-`N_W-1 -: `MEM_ADDR_W] = `shift; //shift
      config_bits[2*`MEMP_CONF_BITS-3*`MEM_ADDR_W-2*`PERIOD_W-`N_W-1 -: `MEM_ADDR_W] = `incr; //incr
      config_bits[2*`MEMP_CONF_BITS-4*`MEM_ADDR_W-3*`PERIOD_W-`N_W-3-1 -: `MEM_ADDR_W] = `iter2; //iter2
      config_bits[2*`MEMP_CONF_BITS-5*`MEM_ADDR_W-3*`PERIOD_W-`N_W-3-1 -: `PERIOD_W] = `per2; //per2
      config_bits[2*`MEMP_CONF_BITS-5*`MEM_ADDR_W-4*`PERIOD_W-`N_W-3-1 -: `MEM_ADDR_W] = `shift2; //shift2
      config_bits[2*`MEMP_CONF_BITS-6*`MEM_ADDR_W-4*`PERIOD_W-`N_W-3-1 -: `MEM_ADDR_W] = `incr2; //incr2

      //run
      run = 1;
      #(clk_per);
      run = 0;

      //wait until done
      do begin

	//compare expected and actual values
	for(i = 0; i < 3; i++) begin
          for(j = 0; j < 3; j++) begin
	    for(k = 0; k < 3; k++) begin
              for(l = 0; l < 3; l++) begin
                #clk_per;
		aux_val = flow_out[2*DATA_W-1 -: DATA_W];
                if(aux_val != pixels[i*5+j+k*5+l]) err_cnt++;
	      end
	    end
          end
        end
      end while(doneA == 0);

      //End simulation
      if(err_cnt == 0) $display("Addr gen and conf bus passed test\n");
      else $display("Addr gen and conf bus failed test with %d errors\n", err_cnt);
      $finish;
   end
	
   always 
     #(clk_per/2) clk = ~clk;
 
   //
   // CPU TASKS
   //
   
   task cpu_write;
      input [`MEM_ADDR_W-1:0] cpu_address;
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
      input [`MEM_ADDR_W-1:0] cpu_address;
      output [DATA_W-1:0] read_reg;
      addr = cpu_address;
      valid = 1;
      we = 0;
      #clk_per;
      read_reg = flow_out[2*DATA_W-1 -: DATA_W]; 
      valid = 0;
      #clk_per;
   endtask
    	 
endmodule
