`timescale 1ns / 1ps

module Float2Int #(
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         running,
    input                         run,
    
    //input / output data
    input [DATA_W-1:0]            in0,
    
    (* versat_latency = 1 *) output reg [DATA_W-1:0] out0
    );

    wire sign = in0[31];
    wire [7:0] exponent = in0[30:23];
    wire [22:0] fraction = in0[22:0];

    // Determine the exponent bias for single precision float
    wire [7:0] exponent_bias = 127;

    // Calculate the biased exponent
    wire signed [7:0] biased_exponent = exponent - exponent_bias;

    // Generate a rounding bit based on the lower bits of the fraction
    wire round_bit = (fraction[0] & fraction[22]);

    // Construct a shifted mantissa (with hidden bit)
    wire [23:0] mantissa = {1'b1, fraction};
    wire [30:0] extra_mantissa = {7'h0,mantissa};

    // Calculate the position to shift right for rounding
    wire [7:0] right_shift_amt = 8'd23 - biased_exponent;
    wire [23:0] right_shifted_mantissa = mantissa >> right_shift_amt;

    wire [7:0] left_shift_amt = biased_exponent - 8'd23;
    wire [30:0] left_shifted_mantissa = extra_mantissa << left_shift_amt;

    reg [31:0] rounded_int;

    // Combine the sign and mantissa to get the rounded integer
    always @* begin
      if(biased_exponent < 0) begin // Smaller than 1.
         if(sign)
            rounded_int = -0;
         else
            rounded_int = 0;
      end else if(right_shift_amt <= 23) begin
         if(sign)
            rounded_int = -{8'h0, right_shifted_mantissa[23:0]};
         else
            rounded_int = {8'h0, right_shifted_mantissa[23:0]};
      end else if(left_shift_amt < 8) begin
         if(sign)
            rounded_int = -{1'b0,left_shifted_mantissa[30:0]};
         else
            rounded_int = {1'b0,left_shifted_mantissa[30:0]};
      end else // Bigger than possible to represent with int. Do not know the actual value to return, the conversion done in C is to return maximum negative number (even if positive) and so it's the implementation we choose to follow
         rounded_int = {1'b1,31'd0};
    end
         
    always @(posedge clk,posedge rst)
    begin
        if (rst) begin
            out0 <= 0;
        end else begin
            out0 <= rounded_int;
        end
    end

endmodule