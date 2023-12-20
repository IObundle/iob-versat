`timescale 1ns / 1ps

// Log2 number of states
`define STATES_W 2

// FSM States
`define IDLE `STATES_W'h0
`define EXT2INT `STATES_W'h1
`define INT2EXT `STATES_W'h2

module ext_addrgen #(
   parameter AXI_ADDR_W   = 32,
   parameter DATA_W       = 32,
   parameter EXT_ADDR_W   = 10,
   parameter EXT_PERIOD_W = 10,
   parameter IO_SIZE_W    = 11,
   parameter MEM_ADDR_W   = 10
) (
   input clk,
   input rst,

   // Control I/F
   input      run,
   output reg done,

   // Configuration
   input        [    AXI_ADDR_W-1:0] ext_addr_i,
   input        [    MEM_ADDR_W-1:0] int_addr_i,
   input        [     IO_SIZE_W-1:0] size_i,
   input        [               1:0] direction_i,
   input        [EXT_PERIOD_W - 1:0] iterations_i,
   input        [EXT_PERIOD_W - 1:0] period_i,
   input        [EXT_PERIOD_W - 1:0] duty_i,
   input        [EXT_PERIOD_W - 1:0] delay_i,
   input        [  EXT_ADDR_W - 1:0] start_i,
   input signed [  EXT_ADDR_W - 1:0] shift_i,
   input signed [  EXT_ADDR_W - 1:0] incr_i,

   // Databus interface
   input                       databus_ready,
   output reg                  databus_valid,
   output     [AXI_ADDR_W-1:0] databus_addr,
   input      [    DATA_W-1:0] databus_rdata,
   output     [    DATA_W-1:0] databus_wdata,
   output reg [  DATA_W/8-1:0] databus_wstrb,

   // internal memory interface
   output                  req_o,
   output                  rnw_o,
   output [MEM_ADDR_W-1:0] addr_o,
   output [    DATA_W-1:0] data_out_o,
   input  [    DATA_W-1:0] data_in_i
);

   // addr_o gen wires
   wire                        [EXT_ADDR_O_I_W-1:0] addr_o_gen;
   wire ready = |iterations_i;
   wire                                             mem_en;

   // int mem regs
   reg [MEM_ADDR_O_W-1:0] counter_int, counter_int_nxt;

   reg                    req_o_int;
   reg                    req_o_int_reg;
   reg                    rnw_o_int;
   reg                    rnw_o_int_reg;
   reg [MEM_ADDR_O_W-1:0] addr_o_int;
   reg [MEM_ADDR_O_W-1:0] addr_o_int_reg;

   // link int mem and ext databus
   assign data_out_o     = databus_rdata;
   assign databus_wdata  = data_in_i;
   /* verilator lint_off WIDTH */assign databus_addr_o = ext_addr_o_i + (addr_o_gen << 2);
   assign addr_o         = int_addr_o_i + counter_int;

   //assign req_o = (direction_i == 2'b01) ? req_o_int_reg : req_o_int;
   //assign rnw_o = (direction_i == 2'b01) ? rnw_o_int_reg : rnw_o_int;

   assign req_o          = req_o_int;
   assign rnw_o          = rnw_o_int;

   // instantiate addr_ogen for ext mem access
   xaddr_ogen #(
      .MEM_ADDR_O_W(EXT_ADDR_O_I_W),
      .PERIOD_I_W  (EXT_PERIOD_I_W)
   ) addr_ogen (
      .clk         (clk),
      .rst         (rst),
      .init        (run),
      .run         (run & ready),
      .pause       (~databus_ready & databus_valid),
      .iterations_i(iterations_i),
      .period_i    (period_i),
      .duty_i      (duty_i),
      .delay_i     (delay_i),
      .start_i     (start_i),
      .shift_i     (shift_i),
      .incr_i      (incr_i),
      .addr_o      (addr_o_gen),
      .mem_en      (mem_en),
      .done        ()
   );

   // State and counter registers
   reg [`STATES_W-1:0] state, state_nxt;
   always @(posedge rst, posedge clk)
      if (rst) begin
         state          <= `IDLE;
         counter_int    <= {MEM_ADDR_O_W{1'b0}};
         req_o_int_reg  <= 0;
         rnw_o_int_reg  <= 1;
         addr_o_int_reg <= 0;
      end else begin
         state          <= state_nxt;
         counter_int    <= counter_int_nxt;
         req_o_int_reg  <= req_o_int;
         rnw_o_int_reg  <= rnw_o_int;
         addr_o_int_reg <= addr_o_int;
      end

   // State machine
   always @* begin
      state_nxt       = state;
      done            = 1'b1;
      req_o_int       = 1'b0;
      rnw_o_int       = 1'b1;
      databus_valid   = 1'b0;
      databus_wstrb   = {(DATA_W / 8) {1'b0}};
      counter_int_nxt = counter_int;
      case (state)
         `IDLE: begin
            counter_int_nxt = {MEM_ADDR_O_W{1'b0}};
            if (run) begin
               if (direction_i == 2'b01) state_nxt = `EXT2INT;
               else if (direction_i == 2'b10) state_nxt = `INT2EXT;
            end
         end
         `EXT2INT: begin
            if (mem_en == 1'b0) state_nxt = `IDLE;
            else begin
               databus_valid = 1'b1;
               done          = 1'b0;
               if (databus_ready) begin
                  req_o_int       = 1'b1;
                  rnw_o_int       = 1'b0;
                  counter_int_nxt = counter_int + 1'b1;
               end
            end
         end
         `INT2EXT: begin
            if (mem_en == 1'b0) state_nxt = `IDLE;
            else begin
               req_o_int     = 1'b1;
               databus_valid = 1'b1;
               databus_wstrb = {(DATA_W / 8) {1'b1}};
               done          = 1'b0;
               if (databus_ready) counter_int_nxt = counter_int + 1'b1;
            end
         end
         default: begin
            state_nxt = `IDLE;
         end
      endcase
   end

endmodule  // ext_addrgen
