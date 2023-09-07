`timescale 1ns / 1ps
`include "AXIInfo.vh"

// Given the initial byte offset, this module aligns incoming data
// Start must be asserted once before the first valid data in a new burst transfer
module burst_align #(
    parameter AXI_DATA_W = 32
    )
    (
        input [OFFSET_W-1:0] offset,
        input start,

        input burst_last, // Assert if on the last cycle of a burst. Most likely is gonna be (xvalid && xready && xlast)
        input transfer_last, // Assert if performing the last transfer of a set of transfers.

        output last_transfer,
        output empty,

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

reg valid;
reg flush;

assign empty = !valid_out; // Since we are always trying to flush, empty is equal to not having anythig to flush away
assign last_transfer = flush;

reg [AXI_DATA_W-1:0] stored_data;

generate
if(AXI_DATA_W == 16) begin
always @* begin
    data_out = 0;
    
    case(offset)
    1'b0: data_out = stored_data; 
    1'b1: data_out = {data_in[7:0],stored_data[15:8]};
    endcase
end
end // if(AXI_DATA_W == 32)
if(AXI_DATA_W == 32) begin
always @* begin
    data_out = 0;

    case(offset)
    2'b00: data_out = stored_data; 
    2'b01: data_out = {data_in[7:0],stored_data[31:8]};
    2'b10: data_out = {data_in[15:0],stored_data[31:16]};
    2'b11: data_out = {data_in[23:0],stored_data[31:24]};
    endcase
end
end // if(AXI_DATA_W == 32)
if(AXI_DATA_W == 64) begin
always @* begin
    data_out = 0;

    case(offset)
    3'b000: data_out = stored_data; 
    3'b001: data_out = {data_in[7:0],stored_data[63:8]};
    3'b010: data_out = {data_in[15:0],stored_data[63:16]};
    3'b011: data_out = {data_in[23:0],stored_data[63:24]};
    3'b100: data_out = {data_in[31:0],stored_data[63:32]};
    3'b101: data_out = {data_in[39:0],stored_data[63:40]};
    3'b110: data_out = {data_in[47:0],stored_data[63:48]};
    3'b111: data_out = {data_in[55:0],stored_data[63:56]};
    endcase
end
end // if(AXI_DATA_W == 64)
if(AXI_DATA_W == 128) begin
always @* begin
    data_out = 0;

    case(offset)
    4'b0000: data_out = stored_data; 
    4'b0001: data_out = {data_in[7:0],stored_data[127:8]};
    4'b0010: data_out = {data_in[15:0],stored_data[127:16]};
    4'b0011: data_out = {data_in[23:0],stored_data[127:24]};
    4'b0100: data_out = {data_in[31:0],stored_data[127:32]};
    4'b0101: data_out = {data_in[39:0],stored_data[127:40]};
    4'b0110: data_out = {data_in[47:0],stored_data[127:48]};
    4'b0111: data_out = {data_in[55:0],stored_data[127:56]};
    4'b1000: data_out = {data_in[63:0],stored_data[127:64]}; 
    4'b1001: data_out = {data_in[71:0],stored_data[127:72]};
    4'b1010: data_out = {data_in[79:0],stored_data[127:80]};
    4'b1011: data_out = {data_in[87:0],stored_data[127:88]};
    4'b1100: data_out = {data_in[95:0],stored_data[127:96]};
    4'b1101: data_out = {data_in[103:0],stored_data[127:104]};
    4'b1110: data_out = {data_in[111:0],stored_data[127:112]};
    4'b1111: data_out = {data_in[119:0],stored_data[127:120]};
    endcase
end
end // if(AXI_DATA_W == 128)
if(AXI_DATA_W == 256) begin
always @* begin
    data_out = 0;

    case(offset)
    5'b00000: data_out = stored_data; 
    5'b00001: data_out = {data_in[7:0],stored_data[255:8]};
    5'b00010: data_out = {data_in[15:0],stored_data[255:16]};
    5'b00011: data_out = {data_in[23:0],stored_data[255:24]};
    5'b00100: data_out = {data_in[31:0],stored_data[255:32]};
    5'b00101: data_out = {data_in[39:0],stored_data[255:40]};
    5'b00110: data_out = {data_in[47:0],stored_data[255:48]};
    5'b00111: data_out = {data_in[55:0],stored_data[255:56]};
    5'b01000: data_out = {data_in[63:0],stored_data[255:64]}; 
    5'b01001: data_out = {data_in[71:0],stored_data[255:72]};
    5'b01010: data_out = {data_in[79:0],stored_data[255:80]};
    5'b01011: data_out = {data_in[87:0],stored_data[255:88]};
    5'b01100: data_out = {data_in[95:0],stored_data[255:96]};
    5'b01101: data_out = {data_in[103:0],stored_data[255:104]};
    5'b01110: data_out = {data_in[111:0],stored_data[255:112]};
    5'b01111: data_out = {data_in[119:0],stored_data[255:120]};
    5'b10000: data_out = {data_in[127:0],stored_data[255:128]}; 
    5'b10001: data_out = {data_in[135:0],stored_data[255:136]};
    5'b10010: data_out = {data_in[143:0],stored_data[255:144]};
    5'b10011: data_out = {data_in[151:0],stored_data[255:152]};
    5'b10100: data_out = {data_in[159:0],stored_data[255:160]};
    5'b10101: data_out = {data_in[167:0],stored_data[255:168]};
    5'b10110: data_out = {data_in[175:0],stored_data[255:176]};
    5'b10111: data_out = {data_in[183:0],stored_data[255:184]};
    5'b11000: data_out = {data_in[191:0],stored_data[255:192]}; 
    5'b11001: data_out = {data_in[199:0],stored_data[255:200]};
    5'b11010: data_out = {data_in[207:0],stored_data[255:208]};
    5'b11011: data_out = {data_in[215:0],stored_data[255:216]};
    5'b11100: data_out = {data_in[223:0],stored_data[255:224]};
    5'b11101: data_out = {data_in[231:0],stored_data[255:232]};
    5'b11110: data_out = {data_in[239:0],stored_data[255:240]};
    5'b11111: data_out = {data_in[247:0],stored_data[255:248]};
    endcase
end
end // if(AXI_DATA_W == 256)
if(AXI_DATA_W == 512) begin
always @* begin
    data_out = 0;

    case(offset)
    6'b000000: data_out = stored_data; 
    6'b000001: data_out = {data_in[7:0],stored_data[511:8]};
    6'b000010: data_out = {data_in[15:0],stored_data[511:16]};
    6'b000011: data_out = {data_in[23:0],stored_data[511:24]};
    6'b000100: data_out = {data_in[31:0],stored_data[511:32]};
    6'b000101: data_out = {data_in[39:0],stored_data[511:40]};
    6'b000110: data_out = {data_in[47:0],stored_data[511:48]};
    6'b000111: data_out = {data_in[55:0],stored_data[511:56]};
    6'b001000: data_out = {data_in[63:0],stored_data[511:64]}; 
    6'b001001: data_out = {data_in[71:0],stored_data[511:72]};
    6'b001010: data_out = {data_in[79:0],stored_data[511:80]};
    6'b001011: data_out = {data_in[87:0],stored_data[511:88]};
    6'b001100: data_out = {data_in[95:0],stored_data[511:96]};
    6'b001101: data_out = {data_in[103:0],stored_data[511:104]};
    6'b001110: data_out = {data_in[111:0],stored_data[511:112]};
    6'b001111: data_out = {data_in[119:0],stored_data[511:120]};
    6'b010000: data_out = {data_in[127:0],stored_data[511:128]}; 
    6'b010001: data_out = {data_in[135:0],stored_data[511:136]};
    6'b010010: data_out = {data_in[143:0],stored_data[511:144]};
    6'b010011: data_out = {data_in[151:0],stored_data[511:152]};
    6'b010100: data_out = {data_in[159:0],stored_data[511:160]};
    6'b010101: data_out = {data_in[167:0],stored_data[511:168]};
    6'b010110: data_out = {data_in[175:0],stored_data[511:176]};
    6'b010111: data_out = {data_in[183:0],stored_data[511:184]};
    6'b011000: data_out = {data_in[191:0],stored_data[511:192]}; 
    6'b011001: data_out = {data_in[199:0],stored_data[511:200]};
    6'b011010: data_out = {data_in[207:0],stored_data[511:208]};
    6'b011011: data_out = {data_in[215:0],stored_data[511:216]};
    6'b011100: data_out = {data_in[223:0],stored_data[511:224]};
    6'b011101: data_out = {data_in[231:0],stored_data[511:232]};
    6'b011110: data_out = {data_in[239:0],stored_data[511:240]};
    6'b011111: data_out = {data_in[247:0],stored_data[511:248]};
    6'b100000: data_out = {data_in[255:0],stored_data[511:256]};
    6'b100001: data_out = {data_in[263:0],stored_data[511:264]};
    6'b100010: data_out = {data_in[271:0],stored_data[511:272]};
    6'b100011: data_out = {data_in[279:0],stored_data[511:280]};
    6'b100100: data_out = {data_in[287:0],stored_data[511:288]};
    6'b100101: data_out = {data_in[295:0],stored_data[511:296]};
    6'b100110: data_out = {data_in[303:0],stored_data[511:304]};
    6'b100111: data_out = {data_in[311:0],stored_data[511:312]};
    6'b101000: data_out = {data_in[319:0],stored_data[511:320]}; 
    6'b101001: data_out = {data_in[327:0],stored_data[511:328]};
    6'b101010: data_out = {data_in[335:0],stored_data[511:336]};
    6'b101011: data_out = {data_in[343:0],stored_data[511:344]};
    6'b101100: data_out = {data_in[351:0],stored_data[511:352]};
    6'b101101: data_out = {data_in[359:0],stored_data[511:360]};
    6'b101110: data_out = {data_in[367:0],stored_data[511:368]};
    6'b101111: data_out = {data_in[375:0],stored_data[511:376]};
    6'b110000: data_out = {data_in[383:0],stored_data[511:384]}; 
    6'b110001: data_out = {data_in[391:0],stored_data[511:392]};
    6'b110010: data_out = {data_in[399:0],stored_data[511:400]};
    6'b110011: data_out = {data_in[407:0],stored_data[511:408]};
    6'b110100: data_out = {data_in[415:0],stored_data[511:416]};
    6'b110101: data_out = {data_in[423:0],stored_data[511:424]};
    6'b110110: data_out = {data_in[431:0],stored_data[511:432]};
    6'b110111: data_out = {data_in[439:0],stored_data[511:440]};
    6'b111000: data_out = {data_in[447:0],stored_data[511:448]}; 
    6'b111001: data_out = {data_in[455:0],stored_data[511:456]};
    6'b111010: data_out = {data_in[463:0],stored_data[511:464]};
    6'b111011: data_out = {data_in[471:0],stored_data[511:472]};
    6'b111100: data_out = {data_in[479:0],stored_data[511:480]};
    6'b111101: data_out = {data_in[487:0],stored_data[511:488]};
    6'b111110: data_out = {data_in[495:0],stored_data[511:496]};
    6'b111111: data_out = {data_in[503:0],stored_data[511:504]};
    endcase
end
end // if(AXI_DATA_W == 256)
endgenerate

// Logic that does not depend on generate
always @*
begin
    valid_out = 1'b0;

    if(valid & valid_in) // A transfer occured
        valid_out = 1'b1;

    if(flush)
        valid_out = 1'b1;
end

always @(posedge clk,posedge rst)
begin
    if(rst) begin
        stored_data <= 0;
        valid <= 0;
        flush <= 0;
    end else begin
        flush <= 0;

        if(start)
            valid <= 1'b0;

        if(valid_in)
            valid <= 1'b1;

        if(valid_in | (burst_last && transfer_last)) begin
            stored_data <= data_in;
        end

        if(burst_last && transfer_last)
            flush <= 1'b1;
    end
end

endmodule // burst_align
