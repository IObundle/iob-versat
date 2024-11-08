`timescale 1ns / 1ps

module PerformNWrites #(
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter LEN_W      = 8,
   parameter COUNT_W    = 0
)(
   // Connect directly to databus interface
   input                          databus_ready,
   output                         databus_valid,
   output reg [  AXI_ADDR_W-1:0]  databus_addr,
   input      [  AXI_DATA_W-1:0]  databus_rdata,
   output     [  AXI_DATA_W-1:0]  databus_wdata,
   output     [AXI_DATA_W/8-1:0]  databus_wstrb,
   output     [       LEN_W-1:0]  databus_len,
   input                          databus_last,

   // Data interface
   input                         data_valid_i,
   output                        data_ready_o,
   input      [  AXI_DATA_W-1:0] data_data_i,
   output                        data_last_o,

   input            [COUNT_W-1:0] count_i,
   input       [  AXI_ADDR_W-1:0] start_address_i,
   input       [  AXI_ADDR_W-1:0] address_shift_i,
   input       [       LEN_W-1:0] write_length,

   input    run_i,
   output   done_o,

   input    clk_i,
   input    rst_i
);

reg [1:0] state;
reg [  AXI_ADDR_W-1:0] address;

assign databus_wstrb = ~0;
assign databus_len   = write_length;
assign databus_wdata = data_data_i;

assign data_ready_o = databus_ready;
assign databus_valid = data_valid_i;

assign databus_addr = address;

wire count_is_zero;
wire counter_advance;

CountNLoops #(.COUNT_W(COUNT_W)) counter(
      .start_i(run_i),
      .count_i(count_i),

      .advance_i(counter_advance),

      .count_is_zero_o(count_is_zero),

      .clk_i(clk_i),
      .rst_i(rst_i)
   );

assign done_o = (state == 2'b0);

always @(posedge clk_i,posedge rst_i) begin
   if(rst_i) begin
      state <= 0;
      address <= 0;
   end else begin
      case(state)
      2'b00: begin
         if(run_i) begin
            state <= 2'b01;
            address <= start_address_i;
         end
      end
      2'b01: begin
         if(count_is_zero) begin
            state <= 2'b00;
         end else begin
            state <= 2'b10;
         end
      end
      2'b10: begin
         if(databus_valid && databus_ready && databus_last) begin
            address <= address + address_shift_i;
            state <= 2'b01;
         end
      end
      2'b11: begin
      end
      endcase
   end
end

assign counter_advance = (databus_valid && databus_ready && databus_last);
assign data_last_o = (databus_valid && databus_ready && databus_last && count_is_zero);

endmodule