#{set size external.size}

#{for ext external}
#{set i index}
#{if ext.type}
// DP

#{for dp ext.dp}
//wire VERSAT0_ext_dp_write_@{i}_port_@{index};
//wire VERSAT0_ext_dp_enable_@{i}_port_@{index};
//wire [@{dp.bitSize}-1:0] VERSAT0_ext_dp_addr_@{i}_port_@{index};
//wire [@{dp.dataSizeOut}-1:0] VERSAT0_ext_dp_out_@{i}_port_@{index};
//wire [@{dp.dataSizeIn}-1:0] VERSAT0_ext_dp_in_@{i}_port_@{index};
#{end}

   my_dp_asym_ram #(
      .A_DATA_W(@{ext.dp[0].dataSizeOut}),
      .B_DATA_W(@{ext.dp[1].dataSizeOut}),
      .ADDR_W(@{ext.dp[0].bitSize})
   )
   ext_dp_@{i}
   (
      .dinA_i(VERSAT0_ext_dp_out_@{i}_port_0_o),
      .addrA_i(VERSAT0_ext_dp_addr_@{i}_port_0_o),
      .enA_i(VERSAT0_ext_dp_enable_@{i}_port_0_o),
      .weA_i(VERSAT0_ext_dp_write_@{i}_port_0_o),
      .doutA_o(VERSAT0_ext_dp_in_@{i}_port_0_i),

      .dinB_i(VERSAT0_ext_dp_out_@{i}_port_1_o),
      .addrB_i(VERSAT0_ext_dp_addr_@{i}_port_1_o),
      .enB_i(VERSAT0_ext_dp_enable_@{i}_port_1_o),
      .weB_i(VERSAT0_ext_dp_write_@{i}_port_1_o),
      .doutB_o(VERSAT0_ext_dp_in_@{i}_port_1_i),

      .clk_i(clk)
   );
#{else}
// 2P
//wire VERSAT0_ext_2p_write_@{i};
//wire [@{ext.tp.bitSizeOut}-1:0] VERSAT0_ext_2p_addr_out_@{i};
//wire [@{ext.tp.dataSizeOut}-1:0] VERSAT0_ext_2p_data_out_@{i};

//wire VERSAT0_ext_2p_read_@{i};
//wire [@{ext.tp.bitSizeIn}-1:0] VERSAT0_ext_2p_addr_in_@{i};
//wire [@{ext.tp.dataSizeIn}-1:0] VERSAT0_ext_2p_data_in_@{i};

   my_2p_asym_ram #(
     .W_DATA_W(@{ext.tp.dataSizeOut}),
     .R_DATA_W(@{ext.tp.dataSizeIn}),
     .ADDR_W(@{ext.tp.bitSizeIn})
   )
   ext_2p_@{i} 
   (
     // Writting port
     .w_en_i(VERSAT0_ext_2p_write_@{i}_o),
     .w_addr_i(VERSAT0_ext_2p_addr_out_@{i}_o),
     .w_data_i(VERSAT0_ext_2p_data_out_@{i}_o),

     // Reading port
     .r_en_i(VERSAT0_ext_2p_read_@{i}_o),
     .r_addr_i(VERSAT0_ext_2p_addr_in_@{i}_o),
     .r_data_o(VERSAT0_ext_2p_data_in_@{i}_i),

     .clk_i(clk)
   );

#{end}
#{end}