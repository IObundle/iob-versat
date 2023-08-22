#{set size external.size}

#{for ext external}
#{set i index}
#{if ext.type}
// DP

#{for dp ext.dp}
wire ext_dp_write_@{i}_port_@{index};
wire ext_dp_enable_@{i}_port_@{index};
wire [@{dp.bitSize}-1:0] ext_dp_addr_@{i}_port_@{index};
wire [@{dp.dataSizeOut}-1:0] ext_dp_out_@{i}_port_@{index};
wire [@{dp.dataSizeIn}-1:0] ext_dp_in_@{i}_port_@{index};
#{end}

   iob_dp_ram #(
         .DATA_W(@{ext.dp[0].dataSizeOut}),
         .ADDR_W(@{ext.dp[0].bitSize})
   )
   ext_dp_@{i}
   (
      .dinA(ext_dp_out_@{i}_port_0),
      .addrA(ext_dp_addr_@{i}_port_0),
      .enA(ext_dp_enable_@{i}_port_0),
      .weA(ext_dp_write_@{i}_port_0),
      .doutA(ext_dp_in_@{i}_port_0),

      .dinB(ext_dp_out_@{i}_port_1),
      .addrB(ext_dp_addr_@{i}_port_1),
      .enB(ext_dp_enable_@{i}_port_1),
      .weB(ext_dp_write_@{i}_port_1),
      .doutB(ext_dp_in_@{i}_port_1),

      .clk(clk)
   );
#{else}
// 2P
wire ext_2p_write_@{i};
wire [@{ext.tp.bitSizeOut}-1:0] ext_2p_addr_out_@{i};
wire [@{ext.tp.dataSizeOut}-1:0] ext_2p_data_out_@{i};

wire ext_2p_read_@{i};
wire [@{ext.tp.bitSizeIn}-1:0] ext_2p_addr_in_@{i};
wire [@{ext.tp.dataSizeIn}-1:0] ext_2p_data_in_@{i};

#{if ext.tp.dataSizeOut == ext.tp.dataSizeIn}
   iob_2p_ram #(
               .DATA_W(@{ext.tp.dataSizeOut}),
               .ADDR_W(@{ext.tp.bitSizeIn})
   )
#{else}
   iob_2p_asym_ram #(
               .W_DATA_W(@{ext.tp.dataSizeOut}),
               .R_DATA_W(@{ext.tp.dataSizeIn}),
               .MAXADDR_W(@{ext.tp.bitSizeIn})
   )
#{end}
   ext_2p_@{i} 
   (
     // Writting port
     .w_en(ext_2p_write_@{i}),
     .w_addr(ext_2p_addr_out_@{i}),
     .w_data(ext_2p_data_out_@{i}),

     // Reading port
     .r_en(ext_2p_read_@{i}),
     .r_addr(ext_2p_addr_in_@{i}),
     .r_data(ext_2p_data_in_@{i}),

     .clk(clk)
   );

#{end}
#{end}