`timescale 1ns / 1ps

`include "xmemdefs.vh"

module xmem_tb;

   //parameters 
   parameter clk_per = 20;
   parameter DATA_W = 32;
   integer i, j, aux_addr;
   
   //control 
   reg 				clk;
   reg 				rst;
   reg  	        	initA, initB;
   reg				runA, runB;
   wire 			doneA, doneB;

   //controller interface
   reg 				ctr_mem_valid;
   reg 				ctr_we;
   reg [`MEM_ADDR_W-1:0]	ctr_addr;
   reg [DATA_W-1:0] 		ctr_data_in;
   
   //dma interface
   reg 				data_we;
   reg [`MEM_ADDR_W-1:0] 	data_addr;
   reg [DATA_W-1:0] 		data_data_in;
   reg 				data_mem_valid;
   
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
	     .initA(initA),
	     .initB(initB),
	     .runA(runA),
   	     .runB(runB),
             .doneA(doneA),
	     .doneB(doneB),

	     //controller interface
	     .ctr_mem_valid(ctr_mem_valid),
	     .ctr_we(ctr_we),
	     .ctr_addr(ctr_addr),
	     .ctr_data_in(ctr_data_in),
	     
	     //dma interface
 	     .data_we(data_we),
	     .data_addr(data_addr),
	     .data_data_in(data_data_in),
	     .data_mem_valid(data_mem_valid),

	     //input / output data
	     .flow_in(flow_in),
	     .flow_out(flow_out),
	     
	     //configurations
	     .config_bits(config_bits)
	     
	     );

   reg [DATA_W-1:0] aux_val;
   
   initial begin
      
`ifdef DEBUG
      $dumpfile("xmem.vcd");
      $dumpvars();
`endif
      
      //inputs
      clk = 0;
      rst = 1;
      initA = 0;
      initB = 0;
      runA = 0;
      runB = 0;

      ctr_mem_valid = 0;
      ctr_we = 0;
      ctr_addr = 0;
      ctr_data_in = 0;

      data_we = 0;
      data_addr = 0;
      data_data_in = 0;
      data_mem_valid = 0;
     
      flow_in = 0;
      config_bits = 0;
      
      //Global reset (100ns)
      #(clk_per*5) rst = 0;

      //Testing data and control interfaces
      $display("\nTesting data and control interfaces");
      
      //Write values to memory
      $display("Value written in memA: %x", 32'h6789ABCD);
      cpu_data_write(16, 32'h6789ABCD);
      $display("Value written in memB: %x", 32'hF0F0F0F0);
      cpu_ctr_write(10, 32'hF0F0F0F0);

      //Read values back from memory
      cpu_data_read(16, aux_val);
      $display("Value read from memA: %x", aux_val);
      cpu_ctr_read(10, aux_val);
      $display("Value read from memB: %x", aux_val);

      //Testing address generator and configuration bus
      $display("\nTesting address generator and configuration bus");

      //configurations
      //simulate reading first 3x3 block of 5x5 feature map
      config_bits[2*`MEMP_CONF_BITS-1 -: `MEM_ADDR_W] = 3; //iterations
      config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-1 -: `PERIOD_W] = 3; //period
      config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-`PERIOD_W-1 -: `PERIOD_W] = 3; //duty
      config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-1 -: `PERIOD_W] = 0; //delay
      config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-1 -: `MEM_ADDR_W] = 0; //start
      config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-`MEM_ADDR_W-1 -: `MEM_ADDR_W] = 5-3; //shift
      config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-2*`MEM_ADDR_W-1 -: `MEM_ADDR_W] = 1; //incr
      config_bits[2*`MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`N_W-3*`MEM_ADDR_W-`PERIOD_W-1-1-1-1 -: 1] = 1; //output address

      //display addr values that should be generated
      $display("\nExpected addr values");
      aux_addr = 0;
      for(i = 0; i < 3; i++) begin
        for(j = 0; j < 3; j++) begin
          $display("Address %d", aux_addr);
          aux_addr += 1;
        end
        aux_addr += (5-3);
      end
      $display("\nActual addr values");

      //run
      initA = 1;
      runA = 1;
      #(clk_per);
      initA = 0;
      runA = 0;

      //wait until done
      do begin
        $display("Address %d", flow_out[2*DATA_W-1 -: DATA_W]);
        #clk_per;
      end while(doneA == 0);

      //End simulation
      $finish;
   end
	
   always 
     #(clk_per/2) clk = ~clk;
 
   //
   // CPU TASKS
   //
   
   task cpu_data_write;
      input [`MEM_ADDR_W-1:0] cpu_address;
      input [DATA_W-1:0] cpu_data;
      data_addr = cpu_address;
      data_mem_valid = 1;
      data_we = 1;
      data_data_in = cpu_data;
      #clk_per;
      data_we = 0;
      data_mem_valid = 0;
      #clk_per;
   endtask
 
   task cpu_data_read;
      input [`MEM_ADDR_W-1:0] cpu_address;
      output [DATA_W-1:0] read_reg;
      data_addr = cpu_address;
      data_mem_valid = 1;
      data_we = 0;
      #clk_per;
      read_reg = flow_out[2*DATA_W-1 -: DATA_W]; 
      data_mem_valid = 0;
      #clk_per;
   endtask
    	 
   task cpu_ctr_write;
      input [`MEM_ADDR_W-1:0] cpu_address;
      input [DATA_W-1:0] cpu_data;
      ctr_addr = cpu_address;
      ctr_mem_valid = 1;
      ctr_we = 1;
      ctr_data_in = cpu_data;
      #clk_per;
      ctr_we = 0;
      ctr_mem_valid = 0;
      #clk_per;
   endtask
 
   task cpu_ctr_read;
      input [`MEM_ADDR_W-1:0] cpu_address;
      output [DATA_W-1:0] read_reg;
      ctr_addr = cpu_address;
      ctr_mem_valid = 1;
      ctr_we = 0;
      #clk_per;
      read_reg = flow_out[DATA_W-1 -: DATA_W]; 
      ctr_mem_valid = 0;
      #clk_per;
   endtask
    	 
endmodule
