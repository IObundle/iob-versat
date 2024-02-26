`ifndef INCLUDED_AXI_INFO
`define INCLUDED_AXI_INFO

function integer calculate_AXI_OFFSET_W(input integer AXI_DATA_W);
   begin
      calculate_AXI_OFFSET_W = $clog2((AXI_DATA_W / 8));
   end
endfunction

function [2:0] calculate_AXI_AXSIZE(input integer AXI_DATA_W);
   begin
      calculate_AXI_AXSIZE = 0;
      if (AXI_DATA_W >= 8) calculate_AXI_AXSIZE = 3'b000;
      if (AXI_DATA_W >= 16) calculate_AXI_AXSIZE = 3'b001;
      if (AXI_DATA_W >= 32) calculate_AXI_AXSIZE = 3'b010;
      if (AXI_DATA_W >= 64) calculate_AXI_AXSIZE = 3'b011;
      if (AXI_DATA_W >= 128) calculate_AXI_AXSIZE = 3'b100;
      if (AXI_DATA_W >= 256) calculate_AXI_AXSIZE = 3'b101;
      if (AXI_DATA_W >= 512) calculate_AXI_AXSIZE = 3'b110;
      if (AXI_DATA_W >= 1024) calculate_AXI_AXSIZE = 3'b111;
   end
endfunction

`endif
