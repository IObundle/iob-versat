`timescale 1ns / 1ps
`include "AXIInfo.vh"

// TODO: Replace code with generic instantiation for 128,256,512 and 1024

// Given aligned data, splits the data in order to meet byte alignment in a burst transfer starting with offset byte
module burst_split #(
    parameter DATA_W = 32
    )
    (
        input [OFFSET_W-1:0] offset,

        input [DATA_W-1:0] data_in,
        input data_valid,

        output reg [DATA_W-1:0] data_out,

        input clk,
        input rst
    );

localparam OFFSET_W = calculate_AXI_OFFSET_W(DATA_W);

reg [DATA_W-8-1:0] stored_data;
always @(posedge clk,posedge rst)
begin
    if(rst) begin
        stored_data <= 0;
    end else if(data_valid) begin
        stored_data <= data_in[DATA_W-1:8];
    end
end

generate 
if(DATA_W == 32) begin
    always @* begin
        data_out = data_in;
        case(offset)
        2'b00: ;
        2'b01: data_out = {data_in[23:0],stored_data[23:16]};
        2'b10: data_out = {data_in[15:0],stored_data[23:8]};
        2'b11: data_out = {data_in[7:0],stored_data[23:0]};
        endcase
    end
end // if(DATA_W == 32)
if(DATA_W == 64) begin
    always @* begin
        data_out = data_in;
        case(offset)
        3'b000: ;
        3'b001: data_out = {data_in[55:0],stored_data[55:48]};
        3'b010: data_out = {data_in[47:0],stored_data[55:40]};
        3'b011: data_out = {data_in[39:0],stored_data[55:32]};
        3'b100: data_out = {data_in[31:0],stored_data[55:24]};
        3'b101: data_out = {data_in[23:0],stored_data[55:16]};
        3'b110: data_out = {data_in[15:0],stored_data[55:8]};
        3'b111: data_out = {data_in[7:0],stored_data[55:0]};
        endcase        
    end
end // if(DATA_W == 64)
if(DATA_W == 128) begin
    always @* begin
        data_out = data_in;
        case(offset)
        4'b0000: ;
        4'b0001: data_out = {data_in[119:0],stored_data[119:112]};
        4'b0010: data_out = {data_in[111:0],stored_data[119:104]};
        4'b0011: data_out = {data_in[103:0],stored_data[119:96]};
        4'b0100: data_out = {data_in[95:0],stored_data[119:88]};
        4'b0101: data_out = {data_in[87:0],stored_data[119:80]};
        4'b0110: data_out = {data_in[79:0],stored_data[119:72]};
        4'b0111: data_out = {data_in[71:0],stored_data[119:64]};
        4'b1000: data_out = {data_in[63:0],stored_data[119:56]};
        4'b1001: data_out = {data_in[55:0],stored_data[119:48]};
        4'b1010: data_out = {data_in[47:0],stored_data[119:40]};
        4'b1011: data_out = {data_in[39:0],stored_data[119:32]};
        4'b1100: data_out = {data_in[31:0],stored_data[119:24]};
        4'b1101: data_out = {data_in[23:0],stored_data[119:16]};
        4'b1110: data_out = {data_in[15:0],stored_data[119:8]};
        4'b1111: data_out = {data_in[7:0],stored_data[119:0]};
        endcase
    end
end // if(DATA_W == 128)
if(DATA_W == 256) begin
    always @* begin
        data_out = data_in;
        case(offset)
        5'b00000: ;
        5'b00001: data_out = {data_in[247:0],stored_data[247:240]};
        5'b00010: data_out = {data_in[239:0],stored_data[247:232]};
        5'b00011: data_out = {data_in[231:0],stored_data[247:224]};
        5'b00100: data_out = {data_in[223:0],stored_data[247:216]};
        5'b00101: data_out = {data_in[215:0],stored_data[247:208]};
        5'b00110: data_out = {data_in[207:0],stored_data[247:200]};
        5'b00111: data_out = {data_in[199:0],stored_data[247:192]};
        5'b01000: data_out = {data_in[191:0],stored_data[247:184]};
        5'b01001: data_out = {data_in[183:0],stored_data[247:176]};
        5'b01010: data_out = {data_in[175:0],stored_data[247:168]};
        5'b01011: data_out = {data_in[167:0],stored_data[247:160]};
        5'b01100: data_out = {data_in[159:0],stored_data[247:152]};
        5'b01101: data_out = {data_in[151:0],stored_data[247:144]};
        5'b01110: data_out = {data_in[143:0],stored_data[247:136]};
        5'b01111: data_out = {data_in[135:0],stored_data[247:128]};
        5'b10000: data_out = {data_in[127:0],stored_data[247:120]};
        5'b10001: data_out = {data_in[119:0],stored_data[247:112]};
        5'b10010: data_out = {data_in[111:0],stored_data[247:104]};
        5'b10011: data_out = {data_in[103:0],stored_data[247:96]};
        5'b10100: data_out = {data_in[95:0],stored_data[247:88]};
        5'b10101: data_out = {data_in[87:0],stored_data[247:80]};
        5'b10110: data_out = {data_in[79:0],stored_data[247:72]};
        5'b10111: data_out = {data_in[71:0],stored_data[247:64]};
        5'b11000: data_out = {data_in[63:0],stored_data[247:56]};
        5'b11001: data_out = {data_in[55:0],stored_data[247:48]};
        5'b11010: data_out = {data_in[47:0],stored_data[247:40]};
        5'b11011: data_out = {data_in[39:0],stored_data[247:32]};
        5'b11100: data_out = {data_in[31:0],stored_data[247:24]};
        5'b11101: data_out = {data_in[23:0],stored_data[247:16]};
        5'b11110: data_out = {data_in[15:0],stored_data[247:8]};
        5'b11111: data_out = {data_in[7:0],stored_data[247:0]};
        endcase
    end
end // if(DATA_W == 256)
if(DATA_W == 512) begin
    always @* begin
        data_out = data_in;
        case(offset)
        6'b000000: ;
        6'b000001: data_out = {data_in[503:0],stored_data[503:496]};
        6'b000010: data_out = {data_in[495:0],stored_data[503:488]};
        6'b000011: data_out = {data_in[487:0],stored_data[503:480]};
        6'b000100: data_out = {data_in[479:0],stored_data[503:472]};
        6'b000101: data_out = {data_in[471:0],stored_data[503:464]};
        6'b000110: data_out = {data_in[463:0],stored_data[503:456]};
        6'b000111: data_out = {data_in[455:0],stored_data[503:448]};
        6'b001000: data_out = {data_in[447:0],stored_data[503:440]};
        6'b001001: data_out = {data_in[439:0],stored_data[503:432]};
        6'b001010: data_out = {data_in[431:0],stored_data[503:424]};
        6'b001011: data_out = {data_in[423:0],stored_data[503:416]};
        6'b001100: data_out = {data_in[415:0],stored_data[503:408]};
        6'b001101: data_out = {data_in[407:0],stored_data[503:400]};
        6'b001110: data_out = {data_in[399:0],stored_data[503:392]};
        6'b001111: data_out = {data_in[391:0],stored_data[503:384]};
        6'b010000: data_out = {data_in[383:0],stored_data[503:376]};
        6'b010001: data_out = {data_in[375:0],stored_data[503:368]};
        6'b010010: data_out = {data_in[367:0],stored_data[503:360]};
        6'b010011: data_out = {data_in[359:0],stored_data[503:352]};
        6'b010100: data_out = {data_in[351:0],stored_data[503:344]};
        6'b010101: data_out = {data_in[343:0],stored_data[503:336]};
        6'b010110: data_out = {data_in[335:0],stored_data[503:328]};
        6'b010111: data_out = {data_in[327:0],stored_data[503:320]};
        6'b011000: data_out = {data_in[319:0],stored_data[503:312]};
        6'b011001: data_out = {data_in[311:0],stored_data[503:304]};
        6'b011010: data_out = {data_in[303:0],stored_data[503:296]};
        6'b011011: data_out = {data_in[295:0],stored_data[503:288]};
        6'b011100: data_out = {data_in[287:0],stored_data[503:280]};
        6'b011101: data_out = {data_in[279:0],stored_data[503:272]};
        6'b011110: data_out = {data_in[271:0],stored_data[503:264]};
        6'b011111: data_out = {data_in[263:0],stored_data[503:256]};
        6'b100000: data_out = {data_in[255:0],stored_data[503:248]};
        6'b100001: data_out = {data_in[247:0],stored_data[503:240]};
        6'b100010: data_out = {data_in[239:0],stored_data[503:232]};
        6'b100011: data_out = {data_in[231:0],stored_data[503:224]};
        6'b100100: data_out = {data_in[223:0],stored_data[503:216]};
        6'b100101: data_out = {data_in[215:0],stored_data[503:208]};
        6'b100110: data_out = {data_in[207:0],stored_data[503:200]};
        6'b100111: data_out = {data_in[199:0],stored_data[503:192]};
        6'b101000: data_out = {data_in[191:0],stored_data[503:184]};
        6'b101001: data_out = {data_in[183:0],stored_data[503:176]};
        6'b101010: data_out = {data_in[175:0],stored_data[503:168]};
        6'b101011: data_out = {data_in[167:0],stored_data[503:160]};
        6'b101100: data_out = {data_in[159:0],stored_data[503:152]};
        6'b101101: data_out = {data_in[151:0],stored_data[503:144]};
        6'b101110: data_out = {data_in[143:0],stored_data[503:136]};
        6'b101111: data_out = {data_in[135:0],stored_data[503:128]};
        6'b110000: data_out = {data_in[127:0],stored_data[503:120]};
        6'b110001: data_out = {data_in[119:0],stored_data[503:112]};
        6'b110010: data_out = {data_in[111:0],stored_data[503:104]};
        6'b110011: data_out = {data_in[103:0],stored_data[503:96]};
        6'b110100: data_out = {data_in[95:0],stored_data[503:88]};
        6'b110101: data_out = {data_in[87:0],stored_data[503:80]};
        6'b110110: data_out = {data_in[79:0],stored_data[503:72]};
        6'b110111: data_out = {data_in[71:0],stored_data[503:64]};
        6'b111000: data_out = {data_in[63:0],stored_data[503:56]};
        6'b111001: data_out = {data_in[55:0],stored_data[503:48]};
        6'b111010: data_out = {data_in[47:0],stored_data[503:40]};
        6'b111011: data_out = {data_in[39:0],stored_data[503:32]};
        6'b111100: data_out = {data_in[31:0],stored_data[503:24]};
        6'b111101: data_out = {data_in[23:0],stored_data[503:16]};
        6'b111110: data_out = {data_in[15:0],stored_data[503:8]};
        6'b111111: data_out = {data_in[7:0],stored_data[503:0]};
        endcase
    end
end // if(DATA_W == 256)
endgenerate

endmodule
