`timescale 1ns / 1ps

// TODO: Replace code with generic instantiation for 128,256,512 and 1024

// Given aligned data, splits the data in order to meet byte alignment in a burst transfer starting with offset byte
`default_nettype none
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

generate 
if(DATA_W == 32) begin
    reg [23:0] stored_data;
    always @(posedge clk,posedge rst)
    begin
        if(rst) begin
            stored_data <= 0;
        end else begin
            stored_data <= data_in[31:8];
        end
    end

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
    reg [55:0] stored_data;
    always @(posedge clk,posedge rst)
    begin
        if(rst) begin
            stored_data <= 0;
        end else if(data_valid) begin
            stored_data <= data_in[63:8];
        end
    end

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
    reg [119:0] stored_data;
    always @(posedge clk,posedge rst)
    begin
        if(rst) begin
            stored_data <= 0;
        end else if(data_valid) begin
            stored_data <= data_in[119:8];
        end
    end

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
    reg [247:0] stored_data;
    always @(posedge clk,posedge rst)
    begin
        if(rst) begin
            stored_data <= 0;
        end else if(data_valid) begin
            stored_data <= data_in[247:8];
        end
    end

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
endgenerate

endmodule
