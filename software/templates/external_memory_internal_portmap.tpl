#{for ext external}
   #{set i index}
   #{if ext.type}
// DP
   #{for dp ext.dp}
.ext_dp_addr_@{i}_port_@{index}_o(ext_dp_addr_@{i}_port_@{index}_o),
.ext_dp_out_@{i}_port_@{index}_o(ext_dp_out_@{i}_port_@{index}_o),
.ext_dp_in_@{i}_port_@{index}_i(ext_dp_in_@{i}_port_@{index}_i),
.ext_dp_enable_@{i}_port_@{index}_o(ext_dp_enable_@{i}_port_@{index}_o),
.ext_dp_write_@{i}_port_@{index}_o(ext_dp_write_@{i}_port_@{index}_o),
   #{end}
   #{else}
// 2P
.ext_2p_addr_out_@{i}_o(ext_2p_addr_out_@{i}_o),
.ext_2p_addr_in_@{i}_o(ext_2p_addr_in_@{i}_o),
.ext_2p_write_@{i}_o(ext_2p_write_@{i}_o),
.ext_2p_read_@{i}_o(ext_2p_read_@{i}_o),
.ext_2p_data_in_@{i}_i(ext_2p_data_in_@{i}_i),
.ext_2p_data_out_@{i}_o(ext_2p_data_out_@{i}_o),
   #{end}
#{end}
