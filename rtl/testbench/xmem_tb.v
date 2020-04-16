`timescale 1ns / 1ps

`include "xmemdefs.vh"

module xmem_tb;

   //parameters 
   parameter clk_period = 20;
   integer i, j, aux_addr, print_flag = 0;
   
   //control 
   reg 				clk;
   reg 				rst;
   reg  	        	initA, initB;
   reg				runA, runB;
   wire 			doneA, doneB;

   //controller interface
   reg 				ctr_mem_req;
   reg 				ctr_we;
   reg [`MEM_ADDR_W-1:0]	ctr_addr;
   reg [`DATA_W-1:0] 		ctr_data_to_wr;
   
   //dma interface
   reg 				databus_we;
   reg [`MEM_ADDR_W-1:0] 	databus_addr;
   reg [`DATA_W-1:0] 		databus_data_in;
   reg 				databus_mem_req;
   
   //input / output data
   reg [2*`DATABUS_W-1:0] 	flow_in;
   wire [2*`DATA_W-1:0]   	flow_out;
   
   //configurations
   reg [2*`MEMP_CONF_BITS-1:0] 	config_bits;
   
   // Instantiate the Unit Under Test (UUT)
   xmem uut (
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
	     .ctr_mem_req(ctr_mem_req),
	     .ctr_we(ctr_we),
	     .ctr_addr(ctr_addr),
	     .ctr_data_to_wr(ctr_data_to_wr),
	     
	     //dma interface
 	     .databus_we(databus_we),
	     .databus_addr(databus_addr),
	     .databus_data_in(databus_data_in),
	     .databus_mem_req(databus_mem_req),

	     //input / output data
	     .flow_in(flow_in),
	     .flow_out(flow_out),
	     
	     //configurations
	     .config_bits(config_bits)
	     
	     );
   
   initial begin
      
      #100
      
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

      ctr_mem_req = 0;
      ctr_we = 0;
      ctr_addr = 0;
      ctr_data_to_wr = 0;

      databus_we = 0;
      databus_addr = 0;
      databus_data_in = 0;
      databus_mem_req = 0;
     
      flow_in = 0;
      config_bits = 0;
      
      //Global reset (100ns)
      #(clk_period*5) rst = 0;

      //Testing databus and control interfaces
      $display("\nTesting databus and control interfaces");
      
      //Write values to memory
      #clk_period ctr_mem_req = 1;
      ctr_we = 1;
      ctr_addr = 10;
      ctr_data_to_wr = 32'hF0F0F0F0;
      databus_mem_req = 1;
      databus_we = 1;
      databus_addr = 16;
      databus_data_in = 32'h6789ABCD;   
      $display("Value written in memA: %x", 32'h6789ABCD);
      $display("Value written in memB: %x", 32'hF0F0F0F0);

      //Read values back from memory
      #clk_period ctr_mem_req = 0;
      databus_mem_req = 0;
      #clk_period ctr_mem_req = 1;
      ctr_we = 0;
      databus_mem_req = 1;
      databus_we = 0;
      #clk_period ctr_mem_req = 0;
      databus_mem_req = 0;
      $display("Value read from memA: %x", flow_out[2*`DATA_W-1 -: `DATA_W]);
      $display("Value read from memB: %x", flow_out[`DATA_W-1:0]);

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

      //define start address
      initA = 1;
      #(clk_period) initA = 0;

      //run
      runA = 1;
      #(clk_period) runA = 0;

      //wait until done
      #(clk_period) print_flag = 1;
      do
        @(posedge clk) #1;
      while(doneA == 0);
      $finish;

      //End simulation
      $finish;
   end
	
   always 
     #(clk_period/2) clk = ~clk;

   always @ (posedge clk) begin
      if(print_flag)
        $display("Address %d", flow_out[2*`DATA_W-1 -: `DATA_W]);
   end
      	 
endmodule
