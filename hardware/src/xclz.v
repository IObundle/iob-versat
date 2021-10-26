`timescale 1ns / 1ps

module xclz (
	     input [31:0]     data_in,
	     output reg [5:0] data_out
	     );

   always @ * begin
      if (data_in[31] == 1'b1)
	data_out = 6'd0;
      else if (data_in[31:30] == 2'b01)
	data_out = 6'd1;
      else if (data_in[31:29] == 3'b001)
	data_out = 6'd2;
      else if (data_in[31:28] == 4'b0001)
	data_out = 6'd3;
      else if (data_in[31:27] == 5'b00001)
	data_out = 6'd4;
      else if (data_in[31:26] == 6'b000001)
	data_out = 6'd5;
      else if (data_in[31:25] == 7'b0000001)
	data_out = 6'd6;
      else if (data_in[31:24] == 8'b00000001)
	data_out = 6'd7;
      else if (data_in[31:23] == 9'b000000001)
	data_out = 6'd8;
      else if (data_in[31:22] == 10'b0000000001)
	data_out = 6'd9;
      else if (data_in[31:21] == 11'b00000000001)
	data_out = 6'd10;
      else if (data_in[31:20] == 12'b000000000001)
	data_out = 6'd11;
      else if (data_in[31:19] == 13'b0000000000001)
	data_out = 6'd12;
      else if (data_in[31:18] == 14'b00000000000001)
	data_out = 6'd13;
      else if (data_in[31:17] == 15'b000000000000001)
	data_out = 6'd14;
      else if (data_in[31:16] == 16'b0000000000000001)
	data_out = 6'd15;
      else if (data_in[31:15] == 17'b00000000000000001)
	data_out = 6'd16;
      else if (data_in[31:14] == 18'b000000000000000001)
	data_out = 6'd17;
      else if (data_in[31:13] == 19'b0000000000000000001)
	data_out = 6'd18;
      else if (data_in[31:12] == 20'b00000000000000000001)
	data_out = 6'd19;
      else if (data_in[31:11] == 21'b000000000000000000001)
	data_out = 6'd20;
      else if (data_in[31:10] == 22'b0000000000000000000001)
	data_out = 6'd21;
      else if (data_in[31:9] == 23'b00000000000000000000001)
	data_out = 6'd22;
      else if (data_in[31:8] == 24'b000000000000000000000001)
	data_out = 6'd23;
      else if (data_in[31:7] == 25'b0000000000000000000000001)
	data_out = 6'd24;
      else if (data_in[31:6] == 26'b00000000000000000000000001)
	data_out = 6'd25;
      else if (data_in[31:5] == 27'b000000000000000000000000001)
	data_out = 6'd26;
      else if (data_in[31:4] == 28'b0000000000000000000000000001)
	data_out = 6'd27;
      else if (data_in[31:3] == 29'b00000000000000000000000000001)
	data_out = 6'd28;
      else if (data_in[31:2] == 30'b000000000000000000000000000001)
	data_out = 6'd29;
      else if (data_in[31:1] == 31'b0000000000000000000000000000001)
	data_out = 6'd30;
      else if (data_in[31:0] == 32'b00000000000000000000000000000001)
	data_out = 6'd31;
      else data_out = 6'd32;
   end
endmodule
