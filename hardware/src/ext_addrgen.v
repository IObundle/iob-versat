`timescale 1ns / 1ps

`include "xversat.vh"
`include "xmemdefs.vh"
`include "versat-io.vh"

// Log2 number of states
`define STATES_W 2

// FSM States
`define IDLE       `STATES_W'h0
`define EXT2INT    `STATES_W'h1
`define INT2EXT    `STATES_W'h2

module ext_addrgen #(
                     parameter DATA_W=32
		     parameter IO_ADDR_W = `IO_ADDR_W,
		     parameter MEM_ADDR_W = `MEM_ADDR_W,
		     parameter IO_SIZE_W = `IO_SIZE_W,
		     parameter PERIOD_W = `PERIOD_W
                     )
   (
	input                            clk,
	input                            rst,
   
    // Control I/F
    input                            run,
    output                           done,

    // Configuration
    input [IO_ADDR_W-1:0]           ext_addr,
    input [MEM_ADDR_W-1:0]          int_addr,
    input [IO_SIZE_W-1:0]           size,
    input [1:0]                      direction,
    input [MEM_ADDR_W - 1:0]        iterations,
	input [PERIOD_W - 1:0]          period,
	input [PERIOD_W - 1:0]          duty,
	input [PERIOD_W - 1:0]          delay,
	input [MEM_ADDR_W - 1:0]        start,
	input signed [MEM_ADDR_W - 1:0] shift,
	input signed [MEM_ADDR_W - 1:0] incr,

    // Databus interface
    input                            databus_ready,
    output                           databus_valid,
    output [IO_ADDR_W-1:0]          databus_addr,
    input [DATA_W-1:0]               databus_rdata,
    output [DATA_W-1:0]              databus_wdata,
    output reg [DATA_W/8-1:0]        databus_wstrb,

	// internal memory interface
	output                           req,
	output                           rnw,
	output [MEM_ADDR_W-1:0]         addr,
	output [DATA_W-1:0]              data_out,
	input [DATA_W-1:0]               data_in
	);
   
   reg                               status;
   
   reg                               init;
   
   reg [IO_SIZE_W:0]                counter_int;
   reg [IO_SIZE_W:0]                counter_int_nxt;
   
   reg [`STATES_W-1:0]               state;
   reg [`STATES_W-1:0]               state_nxt;

   reg                               databus_valid_int;
   reg                               databus_valid_reg;
   wire [IO_ADDR_W-1:0]             databus_addr_int;
   reg [IO_ADDR_W-1:0]              databus_addr_int_reg;

   reg                               req_int;
   reg                               req_int_reg;
   reg                               rnw_int;
   reg                               rnw_int_reg;
   reg [MEM_ADDR_W-1:0]             addr_int;
   reg [MEM_ADDR_W-1:0]             addr_int_reg;

   wire [MEM_ADDR_W-1:0]            addr_gen;
   wire                              ready = |iterations;

   assign databus_valid = (direction == 2'b01)? databus_valid_int : databus_valid_reg;
   assign databus_addr = (direction == 2'b01)? databus_addr_int : databus_addr_int_reg;
   assign data_out = databus_rdata;
   assign databus_wdata = data_in;
   
   assign req = (direction == 2'b01)? req_int_reg : req_int;
   assign rnw = (direction == 2'b01)? rnw_int_reg : rnw_int;
   assign addr = (direction == 2'b01)? addr_int_reg : addr_int;
   
   assign databus_addr_int = ext_addr + addr_gen;
   assign addr_int = int_addr + counter_int;
   
   assign done = ~status;

   xaddrgen # ( 
		     .MEM_ADDR_W(MEM_ADDR_W),
		     .PERIOD_W(PERIOD_W)
   ) addrgen (
                     .clk(clk),
                     .rst(rst),
                     .init(run),
                     .run(run & ready),
                     .pause(~databus_ready & ~run),
                     .iterations(iterations),
                     .period(period),
                     .duty(duty),
                     .delay(delay),
                     .start(start),
                     .shift(shift),
                     .incr(incr),
                     .addr(addr_gen),
                     .mem_en(),
                     .done()
                     );

   // delays
   always @ (posedge rst, posedge clk)
     if (rst) begin
	    databus_valid_reg <= 0;
	    databus_addr_int_reg <= 0;
        req_int_reg <= 0;
        rnw_int_reg <= 1;
        addr_int_reg <= 0;
     end else begin
	    databus_valid_reg <= databus_valid_int;
	    databus_addr_int_reg <= databus_addr_int;
        req_int_reg <= req_int;
        rnw_int_reg <= rnw_int;
        addr_int_reg <= addr_int;
     end
   
   // Counter registers
   always @ (posedge clk)
     if (init) begin
	    counter_int <= {1'b0, {IO_SIZE_W{1'b0}}};
     end else begin
	    counter_int <= counter_int_nxt;
     end
   
   // State register
   always @ (posedge rst, posedge clk)
     if (rst) begin 
	    state <= `STATES_W'd0;
     end else begin
	    state <= state_nxt;
     end
   
   // State machine
   always @ * begin
      state_nxt = state;
      status = 1'b0;
      
      init = 1'b0;
      
      req_int = 1'b0;
      rnw_int = 1'b1;
      
      databus_valid_int = 1'b0;
      databus_wstrb = {(DATA_W/8){1'b0}};
      
      counter_int_nxt = counter_int;
      
      case (state)
	    `IDLE: begin
	       if (run == 1'b1) begin
	          init = 1'b1;

              if (direction == 2'b01) begin
		         state_nxt = `EXT2INT;
	          end else if (direction == 2'b10) begin
		         state_nxt = `INT2EXT;
	          end
	       end
	    end
	    `EXT2INT: begin
	       if (counter_int == {{1'b0},size}) begin
	          state_nxt = `IDLE;
	       end
           
           databus_valid_int = 1'b1;

	       if (databus_ready == 1'b1) begin
              req_int = 1'b1;
	          rnw_int = 1'b0;
	          
	          counter_int_nxt = counter_int + 1'b1;
	       end
	       
	       status = 1'b1;
	    end
	    `INT2EXT: begin
	       if (counter_int == {{1'b0},size}) begin
	          state_nxt = `IDLE;
	       end
           
           req_int = 1'b1;
           databus_valid_int = 1'b1;
           databus_wstrb = {(DATA_W/8){1'b1}};
	       
	       if (databus_ready == 1'b1) begin
	          counter_int_nxt = counter_int + 1'b1;
	       end
	       
	       status = 1'b1;
	    end
        default: begin
           state_nxt = `IDLE;
        end
      endcase
   end
   
endmodule
