#{for ext external}
   #{set i index}
   #{if ext.type}
// DP
   #{for dp ext.dp}
output [@{dp.bitSize}-1:0]   ext_dp_addr_@{i}_port_@{index},
output [@{dp.dataSizeOut}-1:0]   ext_dp_out_@{i}_port_@{index},
input  [@{dp.dataSizeIn}-1:0]   ext_dp_in_@{i}_port_@{index},
output                ext_dp_enable_@{i}_port_@{index},
output                ext_dp_write_@{i}_port_@{index},
   #{end}
   #{else}
// 2P
output [@{ext.tp.bitSizeOut}-1:0]   ext_2p_addr_out_@{i},
output [@{ext.tp.bitSizeIn}-1:0]   ext_2p_addr_in_@{i},
output                ext_2p_write_@{i},
output                ext_2p_read_@{i},
input  [@{ext.tp.dataSizeOut}-1:0]   ext_2p_data_in_@{i},
output [@{ext.tp.dataSizeIn}-1:0]   ext_2p_data_out_@{i},
   #{end}
#{end}
