`timescale 1ns/1ps

`include "xversat.vh"
`include "xmemdefs.vh"
`include "versat-io.vh"

module vread_tb;

   parameter clk_frequency = `CLK_FREQ;
   parameter clk_per = 1e9/clk_frequency; // clock period in ns

   parameter DATA_W = 32;

   parameter file_mem = "memdata.hex";
   parameter file_size = `FILE_SIZE;
   
   // System signals
   reg                          clk;
   reg 			                rst;

   // Control interface
   reg                          run;
   reg                          doneA;
   reg                          doneB;

   // Databus master interface
   wire                         m_databus_ready;
   wire                         m_databus_valid;
   wire [`IO_ADDR_W-1:0]        m_databus_addr;
   reg [DATA_W-1:0]             m_databus_rdata;
   wire [DATA_W-1:0]            m_databus_wdata;
   wire [DATA_W/8-1:0]          m_databus_wstrb;

   // input / output data
   reg [2*`DATABUS_W-1:0]       flow_in;
   wire [DATA_W-1:0]            flow_out;

   // Configurations
   reg [`VI_CONFIG_BITS-1:0]    config_bits;

   // auxilary signals
   reg [`IO_ADDR_W-1:0]         ext_addr;
   reg [`MEM_ADDR_W-1:0]        int_addr;
   reg [`IO_SIZE_W-1:0]         size;
   reg [`MEM_ADDR_W-1:0]        incrA;
   reg [`MEM_ADDR_W-1:0]        iterA;
   reg [`MEM_ADDR_W-1:0]        shiftA;
   reg [`PERIOD_W-1:0]          perA;
   reg [`PERIOD_W-1:0]          dutyA;
   reg [`MEM_ADDR_W-1:0]        startB;
   reg [`MEM_ADDR_W-1:0]        incrB;
   reg [`MEM_ADDR_W-1:0]        iterB;
   reg [`MEM_ADDR_W-1:0]        shiftB;
   reg [`PERIOD_W-1:0]          perB;
   reg [`PERIOD_W-1:0]          dutyB;
   reg [`PERIOD_W-1:0]          delayB;
   reg                          reverseB;
   reg                          extB;
   reg [`MEM_ADDR_W-1:0]        iter2B;
   reg [`PERIOD_W-1:0]          per2B;
   reg [`MEM_ADDR_W-1:0]        shift2B;
   reg [`MEM_ADDR_W-1:0]        incr2B;

   reg                          m_databus_ready_en;
   reg                          m_databus_ready_int;

   // iterator
   integer                      i;

   // Data Bank
   reg [DATA_W-1:0]             mem [file_size-1:0];

   // Instantiate the Unit Under Test (UUT)
   vread uut (
		      .clk(clk),
		      .rst(rst),

              // Control interface
		      .run(run),
		      .doneA(doneA),
		      .doneB(doneB),

              // Databus master interface
              .databus_ready(m_databus_ready),
              .databus_valid(m_databus_valid),
              .databus_addr(m_databus_addr),
              .databus_rdata(m_databus_rdata),
              .databus_wdata(m_databus_wdata),
              .databus_wstrb(m_databus_wstrb),

		      // Data IO
		      .flow_in(flow_in),
		      .flow_out(flow_out),

              // Configuration bus
              .config_bits(config_bits)
		      );

   initial begin

`ifdef VCD
      $dumpfile("vread.vcd");
      $dumpvars;
`endif

      // Global reset of FPGA
      #100;

      clk = 1;
      rst = 0;

      run = 0;

      m_databus_ready_en = 0;

      flow_in = 0;

      config_bits = 0;

      $readmemh(file_mem,mem,0, (file_size - 1));

      // Global reset
      #(clk_per+1);
      rst = 1;

      #clk_per;
      rst = 0;

      // Configure vread
      ext_addr = `IO_ADDR_W'h00000000;
      int_addr = `MEM_ADDR_W'd512;
      size = 255;
      iterA = 255;
      perA = 1;
      dutyA = 1;
      shiftA = 0;
      incrA = 1;

      startB = 0;
      incrB = 1;
      iterB = 300;
      shiftB = 0;
      perB = 1;
      dutyB = 1;
      delayB = 0;
      reverseB = 0;
      extB = 0;
      iter2B = 0;
      per2B = 0;
      shift2B = 0;
      incr2B = 0;
      
      config_bits = 
          {ext_addr, int_addr, size, iterA, perA, dutyA, shiftA, incrA, iterB, perB, dutyB, startB, shiftB, incrB, delayB, reverseB, extB, iter2B, per2B, shift2B, incr2B};

      // Give Run command
      #clk_per;
      run = 1;

      #clk_per;
      run = 0;

      for(i = 0; i < $max(size, iterB+delayB); i++) begin
         #clk_per;

         if ((i%99) == 0 || (i%100) == 0 || (i%101) == 0) begin
            m_databus_ready_en = 0;
         end else begin
            m_databus_ready_en = 1;
         end
      end

      // Configure vread
      ext_addr = `IO_ADDR_W'h00000400;
      int_addr = `MEM_ADDR_W'd0;
      size = 255;
      iterA = 255;
      perA = 1;
      dutyA = 1;
      shiftA = 0;
      incrA = 1;

      startB = 512;
      incrB = 1;
      iterB = 300;
      shiftB = 0;
      perB = 1;
      dutyB = 1;
      delayB = 0;
      reverseB = 0;
      extB = 0;
      iter2B = 0;
      per2B = 0;
      shift2B = 0;
      incr2B = 0;

      config_bits = 
          {ext_addr, int_addr, size, iterA, perA, dutyA, shiftA, incrA, iterB, perB, dutyB, startB, shiftB, incrB, delayB, reverseB, extB, iter2B, per2B, shift2B, incr2B};

      // Give Run command
      #clk_per;
      run = 1;

      #clk_per;
      run = 0;

      for(i = 0; i < $max(size, iterB+delayB); i++) begin
         #clk_per;

         if (i <= size) begin
            if (flow_out != 2*(i+1)) begin
               $display("Test failed");
               $finish;
            end
         end
      end

      $display("Test completed successfully");
      $finish;
   end

   //
   // Memory port
   //

   // Versat-i reads from the memory
   always @ (posedge clk) begin
      if (m_databus_valid)
        m_databus_rdata <= mem[m_databus_addr];
   end

   // Ready compulation
   always @ (posedge clk) begin
      m_databus_ready_int <= m_databus_valid;
   end

   assign m_databus_ready = m_databus_ready_int & m_databus_ready_en;

   //
   // Clocks
   //

   // system clock
   always #(clk_per/2) clk = ~clk;

   //
   // TASKS
   //

endmodule
