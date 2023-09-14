`timescale 1ns/1ps

module skid_control(
    input in_valid,
    output reg in_ready,
    output in_transfer,

    output reg out_valid,
    input out_ready,
    output out_transfer,

    input valid, // Assert if data is valid (something like a unit that takes a couple of cycles to process data would assert this indicate validity)
    input enable_transfer, // If unit allows transfer to occur

    output reg store_data,
    output reg use_stored_data,

    input clk,
    input rst  
  );

assign in_transfer = (in_valid && in_ready);
assign out_transfer = (out_valid && out_ready && enable_transfer);

reg has_stored;

always @(posedge clk,posedge rst)
begin
  if(rst) begin
    has_stored <= 1'b0;
    in_ready <= 1'b1;
  end else begin
    case({!enable_transfer,has_stored,in_valid,out_ready})
    4'b0000:begin
      in_ready <= 1'b1;
    end
    4'b0001:begin
      in_ready <= 1'b1;
    end
    4'b0010:begin
      has_stored <= 1'b1;
      in_ready <= 1'b0;
    end
    4'b0011:begin
      in_ready <= 1'b1;
    end
    4'b0100:begin
      in_ready <= 1'b0;
    end
    4'b0101:begin
      has_stored <= 1'b0;
      in_ready <= 1'b1;
    end
    4'b0110:begin
      in_ready <= 1'b0;
    end
    4'b0111:begin
      in_ready <= 1'b1;
    end
    4'b1000:begin
      in_ready <= 1'b1;
    end
    4'b1001:begin
      in_ready <= 1'b1;
    end
    4'b1010:begin
      has_stored <= 1'b1;
      in_ready <= 1'b0;
    end
    4'b1011:begin
      has_stored <= 1'b1;
      in_ready <= 1'b0;
    end
    4'b1100:begin
      in_ready <= 1'b0;
    end
    4'b1101:begin 
      in_ready <= 1'b0;
    end
    4'b1110:begin
      in_ready <= 1'b0;
    end
    4'b1111:begin
      in_ready <= 1'b0;
    end
    endcase
  end
end

always @*
begin
  out_valid = 1'b0;
  store_data = 1'b0;
  use_stored_data = has_stored;

  case({!enable_transfer,has_stored,in_valid,out_ready})
  4'b0000:begin
  end
  4'b0001:begin
  end
  4'b0010:begin
    out_valid = valid;
  end
  4'b0011:begin
    out_valid = valid;
  end
  4'b0100:begin
    out_valid = valid;
  end
  4'b0101:begin
    out_valid = valid;
  end
  4'b0110:begin
    out_valid = valid;
  end
  4'b0111:begin
    out_valid = valid;
  end
  4'b1000:begin
  end
  4'b1001:begin
  end
  4'b1010:begin
    out_valid = valid;
  end
  4'b1011:begin
    out_valid = valid;
  end
  4'b1100:begin
    out_valid = valid;
  end
  4'b1101:begin
    out_valid = valid;
  end
  4'b1110:begin
    out_valid = valid;
  end
  4'b1111:begin
    out_valid = valid;
  end
  endcase  
end

endmodule
