#{for ext external}
   #{set i index}
   #{if ext.type}
// DP
     #{for dp ext.dp}
   wire [@{dp.bitSize}-1:0]   ext_dp_addr_@{i}_port_@{index}_o;
   wire [@{dp.dataSizeOut}-1:0]   ext_dp_out_@{i}_port_@{index}_o;
   wire [@{dp.dataSizeIn}-1:0]   ext_dp_in_@{i}_port_@{index}_i;
   wire ext_dp_enable_@{i}_port_@{index}_o;
   wire ext_dp_write_@{i}_port_@{index}_o;

   wire [@{dp.bitSize}-1:0]   VERSAT0_ext_dp_addr_@{i}_port_@{index}_o;
   wire [@{dp.dataSizeOut}-1:0]   VERSAT0_ext_dp_out_@{i}_port_@{index}_o;
   wire [@{dp.dataSizeIn}-1:0]   VERSAT0_ext_dp_in_@{i}_port_@{index}_i;
   wire VERSAT0_ext_dp_enable_@{i}_port_@{index}_o;
   wire VERSAT0_ext_dp_write_@{i}_port_@{index}_o;

   assign VERSAT0_ext_dp_addr_@{i}_port_@{index}_o = ext_dp_addr_@{i}_port_@{index}_o;
   assign VERSAT0_ext_dp_out_@{i}_port_@{index}_o = ext_dp_out_@{i}_port_@{index}_o;
   assign ext_dp_in_@{i}_port_@{index}_i = VERSAT0_ext_dp_in_@{i}_port_@{index}_i;
   assign VERSAT0_ext_dp_enable_@{i}_port_@{index}_o = ext_dp_enable_@{i}_port_@{index}_o;
   assign VERSAT0_ext_dp_write_@{i}_port_@{index}_o = ext_dp_write_@{i}_port_@{index}_o;

     #{end}
   #{else}
// 2P
   wire [@{ext.tp.bitSizeOut}-1:0]   ext_2p_addr_out_@{i}_o;
   wire [@{ext.tp.bitSizeIn}-1:0]   ext_2p_addr_in_@{i}_o;
   wire ext_2p_write_@{i}_o;
   wire ext_2p_read_@{i}_o;
   wire [@{ext.tp.dataSizeIn}-1:0]   ext_2p_data_in_@{i}_i;
   wire [@{ext.tp.dataSizeOut}-1:0]   ext_2p_data_out_@{i}_o;

   wire [@{ext.tp.bitSizeOut}-1:0]   VERSAT0_ext_2p_addr_out_@{i}_o;
   wire [@{ext.tp.bitSizeIn}-1:0]   VERSAT0_ext_2p_addr_in_@{i}_o;
   wire VERSAT0_ext_2p_write_@{i}_o;
   wire VERSAT0_ext_2p_read_@{i}_o;
   wire [@{ext.tp.dataSizeIn}-1:0]  VERSAT0_ext_2p_data_in_@{i}_i;
   wire [@{ext.tp.dataSizeOut}-1:0]  VERSAT0_ext_2p_data_out_@{i}_o;

   assign VERSAT0_ext_2p_addr_out_@{i}_o = ext_2p_addr_out_@{i}_o;
   assign VERSAT0_ext_2p_addr_in_@{i}_o = ext_2p_addr_in_@{i}_o;
   assign VERSAT0_ext_2p_write_@{i}_o = ext_2p_write_@{i}_o;
   assign VERSAT0_ext_2p_read_@{i}_o = ext_2p_read_@{i}_o;
   assign ext_2p_data_in_@{i}_i = VERSAT0_ext_2p_data_in_@{i}_i;
   assign VERSAT0_ext_2p_data_out_@{i}_o = ext_2p_data_out_@{i}_o;
   
  #{end}
#{end}
