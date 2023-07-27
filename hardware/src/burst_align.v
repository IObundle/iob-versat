`timescale 1ns / 1ps

// Given the initial byte offset, this module aligns incoming data
// Start must be asserted once before the first valid data in a new burst transfer
`default_nettype none
module burst_align #(
    parameter AXI_DATA_W = 32
    )
    (
        offset,
        input start,

        input last, // After ending the transfer, assert this once so the unit can flush the stored data inside

        // Simple interface for data_in
        input [AXI_DATA_W-1:0] data_in,
        input valid_in,
        //output ready_in,

        // Simple interface for data_out
        output reg [AXI_DATA_W-1:0] data_out,
        output reg valid_out,
        //input      ready_out,

        input clk,
        input rst
    );

localparam OFFSET_W = calculate_AXI_OFFSET_W(AXI_DATA_W);
input [OFFSET_W-1:0] offset;

reg valid;
reg [31:0] stored_data;

always @*
begin
    data_out = 0;
    valid_out = 1'b0;

    case(offset)
    2'b00: data_out = stored_data; 
    2'b01: data_out = {data_in[7:0],stored_data[23:0]};
    2'b10: data_out = {data_in[15:0],stored_data[15:0]};
    2'b11: data_out = {data_in[23:0],stored_data[7:0]};
    endcase

    if(valid & valid_in) // A transfer occured
        valid_out = 1'b1;

    if(last)
        valid_out = 1'b1;
end

always @(posedge clk,posedge rst)
begin
    if(rst) begin
        stored_data <= 0;
        valid <= 0;
    end else begin
        if(start)
            valid <= 1'b0;

        if(valid_in)
            valid <= 1'b1;

        if(valid_in | last) begin
            case(offset)
            2'b00: stored_data <= data_in;
            2'b01: stored_data[23:0] <= data_in[31:8];
            2'b10: stored_data[15:0] <= data_in[31:16];
            2'b11: stored_data[7:0] <= data_in[31:24];
            endcase
        end
    end
end

endmodule // burst_align
