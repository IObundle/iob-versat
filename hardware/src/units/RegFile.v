`timescale 1ns / 1ps

(* source *) module RegFile #(
   parameter DELAY_W = 32,
   parameter NUMBER_REGS = 16,
   parameter ADDR_W  = 6, // 2 + $CLOG(NUMBER_REGS)
   parameter DATA_W  = 32
) (
   //control
   input clk,
   input rst,

   input      running,
   input      run,
   output     done,

   // native interface 
   input      [  ADDR_W-1:0] addr,
   input      [DATA_W/8-1:0] wstrb,
   input      [  DATA_W-1:0] wdata,
   input                     valid,
   output                    rvalid,
   output     [  DATA_W-1:0] rdata,

   //input / output data
   input [DATA_W-1:0] in0,
   output [DATA_W-1:0] out0,
   output [DATA_W-1:0] out1,

   input [3:0]         selectedInput,
   input [3:0]         selectedOutput0,
   input [3:0]         selectedOutput1,
   input disabled,
   
   input [DELAY_W-1:0] delay0
);

wire [DATA_W-1:0] inputs[NUMBER_REGS-1:0];
wire [DATA_W-1:0] outputs[NUMBER_REGS-1:0];
wire [DATA_W-1:0] reg_rdata[NUMBER_REGS-1:0];
wire [NUMBER_REGS-1:0] reg_rvalid;
wire [NUMBER_REGS-1:0] reg_done;

reg [3:0] validRead;

assign rdata = reg_rdata[validRead];

assign done = &reg_done; 
assign rvalid = reg_rvalid[validRead]; 

assign inputs[selectedInput] = in0;
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
      .in0(inputs[i]),
      .out0(outputs[i]),

      .delay0(delay0),

      .disabled(disabled || (selectedInput != i[3:0])),
      .currentValue()
      );
end
endgenerate

endmodule
