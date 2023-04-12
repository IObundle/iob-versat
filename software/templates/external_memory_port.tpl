#{for ext external}
   #{set i index}
   #{if ext.type}
// DP
   #{for port 2}
output [ADDR_W-1:0]   ext_dp_addr_@{i}_port_@{port},
output [DATA_W-1:0]   ext_dp_out_@{i}_port_@{port},
input  [DATA_W-1:0]   ext_dp_in_@{i}_port_@{port},
output                ext_dp_enable_@{i}_port_@{port},
output                ext_dp_write_@{i}_port_@{port},
   #{end}
   #{else}
// 2P
output [ADDR_W-1:0]   ext_2p_addr_out_@{i},
output [ADDR_W-1:0]   ext_2p_addr_in_@{i},
output                ext_2p_write_@{i},
output                ext_2p_read_@{i},
input  [DATA_W-1:0]   ext_2p_data_in_@{i},
output [DATA_W-1:0]   ext_2p_data_out_@{i},
   #{end}
#{end}
