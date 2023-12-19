#{for ext external}
   #{set i index}
   #{if ext.type}
// DP
   #{for dp ext.dp}
.VERSAT0_ext_dp_addr_@{i}_port_@{index}_o(VERSAT0_ext_dp_addr_@{i}_port_@{index}),
.VERSAT0_ext_dp_out_@{i}_port_@{index}_o(VERSAT0_ext_dp_out_@{i}_port_@{index}),
.VERSAT0_ext_dp_in_@{i}_port_@{index}_i(VERSAT0_ext_dp_in_@{i}_port_@{index}),
.VERSAT0_ext_dp_enable_@{i}_port_@{index}_o(VERSAT0_ext_dp_enable_@{i}_port_@{index}),
.VERSAT0_ext_dp_write_@{i}_port_@{index}_o(VERSAT0_ext_dp_write_@{i}_port_@{index}),
   #{end}
   #{else}
// 2P
.VERSAT0_ext_2p_addr_out_@{i}_o(VERSAT0_ext_2p_addr_out_@{i}),
.VERSAT0_ext_2p_addr_in_@{i}_o(VERSAT0_ext_2p_addr_in_@{i}),
.VERSAT0_ext_2p_write_@{i}_o(VERSAT0_ext_2p_write_@{i}),
.VERSAT0_ext_2p_read_@{i}_o(VERSAT0_ext_2p_read_@{i}),
.VERSAT0_ext_2p_data_in_@{i}_i(VERSAT0_ext_2p_data_in_@{i}),
.VERSAT0_ext_2p_data_out_@{i}_o(VERSAT0_ext_2p_data_out_@{i}),
   #{end}
#{end}
