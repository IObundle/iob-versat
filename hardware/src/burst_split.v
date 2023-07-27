`timescale 1ns / 1ps

// TODO: Replace code with generic instantiation for 128,256,512 and 1024

// Given aligned data, splits the data in order to meet byte alignment in a burst transfer starting with offset byte
`default_nettype none
module burst_split #(
    parameter AXI_DATA_W = 32
    )
    (
        offset,
        input firstValid,

        // Simple interface for data_in
        input [AXI_DATA_W-1:0] data_in,
        output ready_in,

        // Simple interface for data_out
        output reg [AXI_DATA_W-1:0] data_out,
        input ready_out,

        input clk,
        input rst
    );

localparam OFFSET_W = calculate_AXI_OFFSET_W(AXI_DATA_W);
input [OFFSET_W-1:0] offset;

assign ready_in = firstValid | ready_out;

generate 
if(AXI_DATA_W == 16) begin
    reg [7:0] stored_data;
    always @(posedge clk,posedge rst)
    begin
        if(rst) begin
            stored_data <= 0;
            data_out <= 0;
        end else begin
            if(firstValid | ready_out) begin
                case(offset)
                1'b0:;
                1'b1: stored_data[7:0] <= data_in[15:8];
                endcase
                case(offset)
                1'b0: data_out <= data_in;
                2'b1: data_out <= {data_in[7:0],stored_data};
                endcase
            end
        end
    end
end // if(AXI_DATA_W == 32)
if(AXI_DATA_W == 32) begin
    reg [23:0] stored_data;
    always @(posedge clk,posedge rst)
    begin
        if(rst) begin
            stored_data <= 0;
            data_out <= 0;
        end else begin
            if(firstValid | ready_out) begin
                case(offset)
                2'b00:;
                2'b01: stored_data[7:0] <= data_in[31:24];
                2'b10: stored_data[15:0] <= data_in[31:16];
                2'b11: stored_data <= data_in[31:8];
                endcase
                case(offset)
                2'b00: data_out <= data_in;
                2'b01: data_out <= {data_in[23:0],stored_data[7:0]};
                2'b10: data_out <= {data_in[15:0],stored_data[15:0]};
                2'b11: data_out <= {data_in[7:0],stored_data[23:0]};
                endcase
            end
        end
    end
end // if(AXI_DATA_W == 32)
if(AXI_DATA_W == 64) begin
    reg [55:0] stored_data;
    always @(posedge clk,posedge rst)
    begin
        if(rst) begin
            stored_data <= 0;
            data_out <= 0;
        end else begin
            if(firstValid | ready_out) begin
                case(offset)
                3'b000:;
                3'b001: stored_data[7:0] <= data_in[63:56];
                3'b010: stored_data[15:0] <= data_in[63:48];
                3'b011: stored_data[23:0] <= data_in[63:40];
                3'b100: stored_data[31:0] <= data_in[63:32];
                3'b101: stored_data[39:0] <= data_in[63:24];
                3'b110: stored_data[47:0] <= data_in[63:16];
                3'b111: stored_data <= data_in[63:8];
                endcase
                case(offset)
                3'b000: data_out <= data_in;
                3'b001: data_out <= {data_in[55:0],stored_data[7:0]};
                3'b010: data_out <= {data_in[47:0],stored_data[15:0]};
                3'b011: data_out <= {data_in[39:0],stored_data[23:0]};
                3'b100: data_out <= {data_in[31:0],stored_data[31:0]};
                3'b101: data_out <= {data_in[23:0],stored_data[39:0]};
                3'b110: data_out <= {data_in[15:0],stored_data[47:0]};
                3'b111: data_out <= {data_in[7:0],stored_data[55:0]};
                endcase
            end
        end
    end
end // if(AXI_DATA_W == 64)

endgenerate

endmodule
