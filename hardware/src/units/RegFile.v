`timescale 1ns / 1ps

(* source *) module RegFile #(
   parameter DELAY_W = 7,
   parameter NUMBER_REGS = 16,
   parameter NUMBER_REGS_W = 4,
   parameter ADDR_W  = 6, // 2 + $CLOG(NUMBER_REGS)
   parameter DATA_W  = 32,
   parameter AXI_DATA_W = 32
) (
   //control
   input clk,
   input rst,

   input      running,
   input      run,
   output     done,

   output [    ADDR_W-1:0] ext_2p_addr_out_0,
   output [    ADDR_W-1:0] ext_2p_addr_in_0,
   output                  ext_2p_write_0,
   output                  ext_2p_read_0,
   input  [AXI_DATA_W-1:0] ext_2p_data_in_0,
   output [AXI_DATA_W-1:0] ext_2p_data_out_0,

   output [    ADDR_W-1:0] ext_2p_addr_out_1,
   output [    ADDR_W-1:0] ext_2p_addr_in_1,
   output                  ext_2p_write_1,
   output                  ext_2p_read_1,
   input  [AXI_DATA_W-1:0] ext_2p_data_in_1,
   output [AXI_DATA_W-1:0] ext_2p_data_out_1,

   // native interface 
   input      [  ADDR_W-1:0] addr,
   input      [DATA_W/8-1:0] wstrb,
   input      [  DATA_W-1:0] wdata,
   input                     valid,
   output                    rvalid,
   output     [  DATA_W-1:0] rdata,

   //input / output data
   input [DATA_W-1:0] in0,
   (* versat_latency = 2 *) output [DATA_W-1:0] out0,
   (* versat_latency = 2 *) output [DATA_W-1:0] out1,

   input [3:0]         selectedInput,
   input [3:0]         selectedOutput0,
   input [3:0]         selectedOutput1,
   input disabled,
   
   input [DELAY_W-1:0] delay0
);

wire lastCycle;

   DelayCalc delay(
   .clk(clk),
   .rst(rst),

   .running(running),
   .run(run),
   .done(done),

   .lastCycle(lastCycle),

   .delay0(delay0)
   );

reg [ADDR_W-1:0] address0,address1,addressWrite;
reg write,read;

assign ext_2p_addr_out_0 = addressWrite;
assign ext_2p_addr_out_1 = addressWrite;

assign ext_2p_addr_in_0 = address0;
assign ext_2p_addr_in_1 = address1;

assign ext_2p_write_0 = write;
assign ext_2p_write_1 = write;

assign ext_2p_read_0 = 1'b1; //read;
assign ext_2p_read_1 = 1'b1; //read;

assign out0 = ext_2p_data_in_0;
assign out1 = ext_2p_data_in_1;

reg [DATA_W-1:0] dataToWrite;

assign ext_2p_data_out_0 = dataToWrite;
assign ext_2p_data_out_1 = dataToWrite;

reg read_reg,write_reg;

assign rvalid = read_reg | write_reg;
assign rdata = (rvalid ? ext_2p_data_in_0 : 0);

always @(posedge clk,posedge rst) begin
   if(rst) begin
      write <= 1'b0;
      read <= 1'b0;
      read_reg <= 1'b0;
      write_reg <= 0;
      dataToWrite <= 0;
      address0 <= 0;
      address1 <= 0;
   end else begin
      write <= 1'b0;
      read <= 1'b0;
      read_reg <= read;
      write_reg <= 0;

      address0 <= {selectedOutput0,2'b00};
      address1 <= {selectedOutput1,2'b00};

      if(lastCycle && !disabled) begin
         dataToWrite <= in0;
         addressWrite <= {selectedInput,2'b00};
         write <= 1'b1;
      end

      if (valid & (|wstrb)) begin
         dataToWrite <= wdata;
         addressWrite <= addr;
         write <= 1'b1;
      end
      if (valid & (wstrb == 0)) begin
         write_reg <= 1'b1;
         address0 <= addr;
         address1 <= addr;
         read <= 1'b1;
      end
   end
end

// A set amount of registers

//reg [DATA_W-1:0] mem[NUMBER_REGS-1:0];
/*
wire [DATA_W-1:0] inputs[NUMBER_REGS-1:0];
wire [DATA_W-1:0] outputs[NUMBER_REGS-1:0];
wire [DATA_W-1:0] reg_rdata[NUMBER_REGS-1:0];
wire [NUMBER_REGS-1:0] reg_rvalid;
wire [NUMBER_REGS-1:0] reg_done;

reg [3:0] validRead;

assign rdata = reg_rdata[validRead];

assign done = &reg_done; 
assign rvalid = reg_rvalid[validRead]; 

assign out0 = outputs[selectedOutput0];
assign out1 = outputs[selectedOutput1];

integer j;
always @* begin
   validRead = 0;
   for(j = 0; j < NUMBER_REGS; j++) begin
      if(valid && addr[ADDR_W-1:2] == j[3:0]) begin
         validRead = j[3:0];
      end
   end
end

generate
genvar i;
for(i = 0; i < NUMBER_REGS; i = i + 1) begin : genRegs
   Reg register(
      .clk(clk),
      .rst(rst),

      .running(running),
      .run(run),
      .done(reg_done[i]),

      // native interface 
      .addr(addr[1:0]),
      .wstrb(wstrb),
      .wdata(wdata),
      .valid(valid && (addr[ADDR_W-1:2] == i[3:0])),
      .rvalid(reg_rvalid[i]),
      .rdata(reg_rdata[i]),

      //input / output data
      .in0(in0),
      .out0(outputs[i]),

      .delay0(delay0),

      .disabled(disabled || (selectedInput != i[3:0])),
      .currentValue()
      );
end
endgenerate
*/

endmodule
