#{define handleStrobe strobeWireName regName regSize regBitStart dataWireName}
  #{set size info.wire.bitSize}
  #{set wstrb 0}
  #{set wstrbAmount 0}
  #{while size > 0}
    #{set amount 8}
    #{if size < 8} #{set amount size} #{end}
        if(@{strobeWireName}[@{wstrb}]) @{regName}[@{regBitStart + wstrbAmount}+:@{amount}] <= @{dataWireName}[@{wstrbAmount}+:@{amount}];
    #{set size (size - 8)}
    #{inc wstrb}
    #{set wstrbAmount (wstrbAmount + amount)}
  #{end}
#{end}

`timescale 1ns / 1ps

`include "versat_defs.vh"

module versat_configurations #(
   parameter ADDR_W = 32,
   parameter DATA_W = 32,
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = @{arch.databusDataSize},
   parameter LEN_W = 20
)(
   #{if configurationBits}
   output [@{configurationBits-1}:0] config_data_o,
   #{else}
   // No output, accelerator does not contain configuration bits
   #{end}

   input memoryMappedAddr,
   input [DATA_W-1:0] data_write,

   input [ADDR_W-1:0] address,
   input [(DATA_W/8)-1:0] data_wstrb,
   input [DATA_W-1:0] data_data,

   input change_config_pulse,

#{for info wireInfo}
#{if info.isStatic}
   output [@{info.wire.bitSize-1}:0] @{info.wire.name},
#{end}
#{end}

   input clk_i,
   input rst_i
);

// Everything passes through shadow

// Shadow -> Config : for Read stage 
// Shadow -> Compute -> Config : for Compute stage  
// Shadow -> Write -> Compute -> Config : for Write stage

reg [@{configurationBits-1}:0] configdata;

#{if opts.shadowRegister}
reg [@{configurationBits-1}:0] shadow_configdata;
#{end}

assign config_data_o = configdata;

#{set configReg "configdata"}
#{if opts.shadowRegister}
     #{set configReg "shadow_configdata"}
#{end}

#{for info wireInfo} #{set wire info.wire}
    #{if wire.stage == 0 or wire.stage == 2}
reg [@{wire.bitSize-1}:0] Compute_@{wire.name};
    #{end}
    #{if wire.stage == 2}
reg [@{wire.bitSize-1}:0] Write_@{wire.name};
    #{end}
#{end}

#{if configurationBits}
// Shadow writing
always @(posedge clk_i,posedge rst_i)
begin
   if(rst_i) begin
      shadow_configdata <= {@{configurationBits}{1'b0}};
   end else if(data_write & !memoryMappedAddr) begin
      #{for info wireInfo}
      if(address[@{configurationAddressBits + 1}:0] == @{info.addr}) begin // @{info.addr|> Hex} @{info.wire.name}
      #{call handleStrobe "data_wstrb" "shadow_configdata" info.wire.bitSize info.configBitStart "data_data"}
      end
      #{end}
   end
end

always @(posedge clk_i,posedge rst_i)
begin
   if(rst_i) begin
      configdata <= {@{configurationBits}{1'b0}};
   end else if(change_config_pulse) begin
      // Shadow directly to Config (Read stage)
#{for info wireInfo}
  #{if info.wire.stage == 1}
      configdata[@{info.configBitStart}+:@{info.wire.bitSize}] <= shadow_configdata[@{info.configBitStart}+:@{info.wire.bitSize}];
  #{end}
#{end}
      // Compute to Config (Compute stage)
#{for info wireInfo}
  #{if info.wire.stage == 0 or info.wire.stage == 2}
      configdata[@{info.configBitStart}+:@{info.wire.bitSize}] <= Compute_@{info.wire.name};
  #{end}
#{end}
  end
end

// Stores Shadow data into Compute
always @(posedge clk_i,posedge rst_i)
begin
   if(rst_i) begin
#{for info wireInfo}
  #{if info.wire.stage == 0 or info.wire.stage == 2}
      Compute_@{info.wire.name} <= {@{info.wire.bitSize}{1'b0}};
  #{end}
#{end}
   end else if(change_config_pulse) begin
      // Shadow to Compute (Compute stage)
#{for info wireInfo}
  #{if info.wire.stage == 0}
      Compute_@{info.wire.name} <= shadow_configdata[@{info.configBitStart}+:@{info.wire.bitSize}];
  #{end}
#{end}
      // Write to Compute (Write stage)
#{for info wireInfo}
  #{if info.wire.stage == 2}
      Compute_@{info.wire.name} <= Write_@{info.wire.name};
  #{end}
#{end}
  end
end

always @(posedge clk_i,posedge rst_i)
begin
   if(rst_i) begin
#{for info wireInfo}
  #{if info.wire.stage == 2}
      Write_@{info.wire.name} <= {@{info.wire.bitSize}{1'b0}};
  #{end}
#{end}
   end else if(change_config_pulse) begin
      // Stores Shadow data into Write
#{for info wireInfo}
  #{if info.wire.stage == 2}
      Write_@{info.wire.name} <= shadow_configdata[@{info.configBitStart}+:@{info.wire.bitSize}];
  #{end}
#{end}
   end
end

#{for info wireInfo}
#{if info.isStatic}
assign @{info.wire.name} = configdata[@{info.configBitStart}+:@{info.wire.bitSize}];
#{end}
#{end}

endmodule
