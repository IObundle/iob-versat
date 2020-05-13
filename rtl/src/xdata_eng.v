/*

 Data bus structure

 {MEM0A, MEM0B, ..., VREAD0, ..., ALU0, ..., ALULITE0, ..., MUL0, ..., MULADD0, ..., BS0, ...}


 Config bus structure

 {MEM0A, MEM0B, ..., VREAD0, ..., VWRITE0, ..., ALU0, ..., ALULITE0, ..., MUL0, ..., MULADD0, ..., BS0, ...}

 */

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

module xdata_eng # (
		            parameter	  DATA_W = 32
		            )
   (
    input                           clk,
    input                           rst,

    // data/ctr interface
    input                           valid,
    input                           we,
    input [`nMEM_W+`MEM_ADDR_W:0]   addr,
    input [DATA_W-1:0]              rdata,
    output [DATA_W-1:0]             wdata,
`ifdef IO
    // Databus master interface
    input [`nIO-1:0]                m_databus_ready,
    output [`nIO-1:0]               m_databus_valid,
    output [`nIO*`IO_ADDR_W-1:0]    m_databus_addr,
    input [`nIO*`DATAPATH_W-1:0]    m_databus_rdata,
    output [`nIO*`DATAPATH_W-1:0]   m_databus_wdata,
    output [`nIO*`DATAPATH_W/8-1:0] m_databus_wstrb,
`endif
    // flow interface
    input [`DATABUS_W-1:0]          flow_in,
    output [`DATABUS_W-1:0]         flow_out,

    // configuration bus
    input [`CONF_BITS-1:0]          config_bus
    );

   // WIDE ENGINE DATA BUS
   wire [2*`DATABUS_W-1:0]          data_bus;

   // flow interface
   assign data_bus[2*`DATABUS_W-1:`DATABUS_W] = flow_in;
   assign flow_out = data_bus[`DATABUS_W-1:0] ;

   //
   // ADDRESS DECODER
   //

   // address register
   reg [`nMEM_W-1:0] addr_reg;
   always @ (posedge rst, posedge clk)
     if (rst)
	   addr_reg <= 0;
     else
	   addr_reg <= addr[`nMEM_W + `MEM_ADDR_W -1 -: `nMEM_W];

   // select control/status register or data memory
   reg control_valid;
   reg [`nMEM-1:0] mem_valid;
   always @ * begin
      integer j;
      control_valid = 1'b0;
      mem_valid = `nMEM'b0;
      if (addr[`nMEM_W+`MEM_ADDR_W])
        control_valid = valid;
      else
        for (j=0; j<`nMEM; j=j+1)
	      if ( j[`nMEM_W-1:0] == addr[`nMEM_W+`MEM_ADDR_W-1 -: `nMEM_W] )
	        mem_valid[j] = valid;
   end

   // register selected data memory output
   reg [`DATAPATH_W-1: 0] data_reg;
   always @ * begin
      integer j;
      data_reg = {DATA_W{1'b0}};
      for (j=0; j < `nMEM; j= j+1)
	    if (addr_reg == j[`nMEM_W-1:0])
	      data_reg = data_bus[`DATA_MEM0A_B - 2*j*`DATAPATH_W  -: `DATAPATH_W]; //Port A
   end
   
   // read: select output data
   wire [2*`nMEM-1:0] mem_done;
   wire               io_done;
`ifdef IO
   wire [2*`nVI-1:0]  read_port_done;
   wire [2*`nVO-1:0]  write_port_done;
`endif
   wire               done;

`ifdef IO
   assign io_done = &{read_port_done, write_port_done};
`else
   assign io_done = 1'b1;
`endif
   assign done = &{mem_done, io_done};

   assign wdata = (control_valid)? {{DATA_W-1{1'b0}}, done} :
                                   {{(DATA_W-`DATAPATH_W){data_reg[`DATAPATH_W-1]}}, data_reg};

   // run
   reg ctr_reg;
   always @ (posedge rst, posedge clk)
     if (rst)
       ctr_reg <= 1'b0;
     else if (~we | ~control_valid)
       ctr_reg <= 1'b0;
     else
       ctr_reg <= rdata[0];
   wire run = control_valid & we & rdata[0];
   wire run_reg = ctr_reg;
   
   // 
   // CONFIGURATION SHADOW REGISTER
   //
   reg [`CONF_BITS-1:0] config_reg_shadow;
   always @ (posedge rst, posedge clk)
     if (rst)
	   config_reg_shadow <= {`CONF_BITS{1'b0}};
     else if (run)
	   config_reg_shadow <= config_bus;
   
   //
   // INSTANTIATE THE FUNCTIONAL UNITS
   //

   // generate iterator
   genvar                                      i;

   generate for (i=0; i < `nMEM; i=i+1) begin : mem_array
      xmem # (
              .DATA_W(`DATAPATH_W)
              )
      mem (
	       .clk(clk),
	       .rst(rst),

	       .run(run_reg),
	       .doneA(mem_done[2*i]),
	       .doneB(mem_done[2*i+1]),

	       // data/control interface
	       .valid(mem_valid[i]),
	       .we(we),
	       .addr(addr[`MEM_ADDR_W-1:0]),
	       .rdata(rdata[`DATAPATH_W-1:0]),

	       // flow interface
	       .flow_in(data_bus),
	       .flow_out(data_bus[`DATA_MEM0A_B - 2*i*`DATAPATH_W -: 2*`DATAPATH_W]),

	       // configuration interface
	       .config_bits(config_reg_shadow[`CONF_MEM0A_B - 2*i*`MEMP_CONF_BITS -: 2*`MEMP_CONF_BITS])
	       );
   end
   endgenerate

   // Instantiate the read ports
   generate for (i=0; i < `nVI; i=i+1) begin : read_port_array
      vread # (
               .DATA_W(`DATAPATH_W)
               )
      read_port (
	             .clk(clk),
	             .rst(rst),

	             .run(run_reg),
	             .doneA(read_port_done[2*i]),
	             .doneB(read_port_done[2*i+1]),

                 // Databus interface
	             .databus_ready(m_databus_ready[`nIO-1 -i -: 1]),
	             .databus_valid(m_databus_valid[`nIO-1 -i -: 1]),
	             .databus_addr(m_databus_addr[`nIO*`IO_ADDR_W-1 -i*`IO_ADDR_W -: `IO_ADDR_W]),
	             .databus_rdata(m_databus_rdata[`nIO*`DATAPATH_W-1 -i*`DATAPATH_W -: `DATAPATH_W]),
	             .databus_wdata(m_databus_wdata[`nIO*`DATAPATH_W-1 -i*`DATAPATH_W -: `DATAPATH_W]),
                 .databus_wstrb(m_databus_wstrb[`nIO*`DATAPATH_W/8-1 -i*`DATAPATH_W/8 -: `DATAPATH_W/8]),

	             // flow interface
		         .flow_in(data_bus),
		         .flow_out(data_bus[`DATA_VI0_B - i*`DATAPATH_W -: `DATAPATH_W]),

	             // Configuration interface
	             .config_bits(config_reg_shadow[`CONF_VI0_B - i*`VI_CONFIG_BITS -: `VI_CONFIG_BITS])
	             );
   end
   endgenerate

   // Instantiate the write ports
   generate for (i=0; i < `nVO; i=i+1) begin : write_port_array
      vwrite # (
                .DATA_W(`DATAPATH_W)
                )
      write_port (
	              .clk(clk),
	              .rst(rst),

	              .run(run_reg),
	              .doneA(write_port_done[2*i]),
	              .doneB(write_port_done[2*i+1]),

	              // Databus interface
	              .databus_ready(m_databus_ready[`nIO-1 -(i+`nVI) -: 1]),
	              .databus_valid(m_databus_valid[`nIO-1 -(i+`nVI) -: 1]),
	              .databus_addr(m_databus_addr[`nIO*`IO_ADDR_W-1 -(i+`nVI)*`IO_ADDR_W -: `IO_ADDR_W]),
	              .databus_rdata(m_databus_rdata[`nIO*`DATAPATH_W-1 -(i+`nVI)*`DATAPATH_W -: `DATAPATH_W]),
	              .databus_wdata(m_databus_wdata[`nIO*`DATAPATH_W-1 -(i+`nVI)*`DATAPATH_W -: `DATAPATH_W]),
                  .databus_wstrb(m_databus_wstrb[`nIO*`DATAPATH_W/8-1 -(i+`nVI)*`DATAPATH_W/8 -: `DATAPATH_W/8]),

	              // flow interface
		          .flow_in(data_bus),

	              // Configuration interface
	              .config_bits(config_reg_shadow[`CONF_VO0_B - i*`VO_CONFIG_BITS -: `VO_CONFIG_BITS])
	              );
   end
   endgenerate

   //
   // Instantiate the ALUs
   //
   generate for (i=0; i < `nALU; i=i+1) begin : add_array
      xalu # (
              .DATA_W(`DATAPATH_W)		
              )
      alu (
	       .clk(clk),
	       .rst(run_reg),

	       // flow interface
	       .databus_in(data_bus),
	       .flow_out(data_bus[`DATA_ALU0_B - i*`DATAPATH_W -: `DATAPATH_W]),

	       // configuration interface
	       .configdata(config_reg_shadow[`CONF_ALU0_B - i*`ALU_CONF_BITS -: `ALU_CONF_BITS])
           );
   end
   endgenerate

   //
   // Instantiate the ALULITEs
   //
   generate for (i=0; i < `nALULITE; i=i+1) begin : add_LITE_array
      xalulite # (
	              .DATA_W(`DATAPATH_W)
                  )
      aluLITE (
	           .clk(clk),
	           .rst(run_reg),

	           // flow interface
	           .flow_in(data_bus),
	           .flow_out(data_bus[`DATA_ALULITE0_B - i*`DATAPATH_W  -: `DATAPATH_W]),

	           // configuration interface
	           .configdata(config_reg_shadow[`CONF_ALULITE0_B - i*`ALULITE_CONF_BITS -: `ALULITE_CONF_BITS])
	           );
   end
   endgenerate

   //
   // Instantiate the MULs
   //
   generate for (i=0; i < `nMUL; i=i+1) begin : mul_array
      xmul # (
	          .DATA_W(`DATAPATH_W)
              )
      mul (
	       .clk(clk),
	       .rst(run_reg),

	       // flow interface
	       .flow_in(data_bus),
	       .flow_out(data_bus[`DATA_MUL0_B - i*`DATAPATH_W -: `DATAPATH_W]),

	       // configuration interface
	       .configdata(config_reg_shadow[`CONF_MUL0_B - i*`MUL_CONF_BITS -: `MUL_CONF_BITS])
	       );
   end
   endgenerate

   //
   // Instantiate the MULADDs
   //
   generate for (i=0; i < `nMULADD; i=i+1) begin : muladd_array
      xmuladd # (
	             .DATA_W(`DATAPATH_W)
                 )
      muladd (
	          .clk(clk),
	          .rst(run_reg),
              .addrgen_rst(rst),

	          // flow interface
	          .flow_in(data_bus),
	          .flow_out(data_bus[`DATA_MULADD0_B - i*`DATAPATH_W -: `DATAPATH_W]),

    	      // configuration interface
	          .configdata(config_reg_shadow[`CONF_MULADD0_B - i*`MULADD_CONF_BITS -: `MULADD_CONF_BITS])
	          );
   end
   endgenerate

   //
   // Instantiate the BSs
   //
   generate for (i=0; i < `nBS; i=i+1) begin : bs_array
      xbs # (
	         .DATA_W(`DATAPATH_W)
             )
      bs (
	      .clk(clk),
	      .rst(run_reg),

	      // flow interface
	      .flow_in(data_bus),
	      .flow_out(data_bus[`DATA_BS0_B - i*`DATAPATH_W -: `DATAPATH_W]),

	      // configuration interface
	      .configdata(config_reg_shadow[`CONF_BS0_B - i*`BS_CONF_BITS -: `BS_CONF_BITS])
	      );
   end
   endgenerate

endmodule
