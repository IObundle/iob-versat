load_package flow
#load_package ::quartus::incremental_compilation::report


# Create the project if it doesn't exist
if [project_exists versat] {
  project_open versat -revision xversat
} else {
  project_new versat -revision xversat
}

# Set the device, the name of the top-level BDF,
# and the name of the top level entity
set_global_assignment -name FAMILY "Cyclone V"
set_global_assignment -name DEVICE 5CGTFD9E5F35C7
set_global_assignment -name VERILOG_FILE xversat.v
set_global_assignment -name TOP_LEVEL_ENTITY xversat

# Search paths
set_global_assignment -name SEARCH_PATH ../../../verilog_src


# Add pin assignments here
#set_location_assignment -to clk             Pin_H19
set_instance_assignment -to clk -name IO_STANDARD "LVDS"


# compile the project
execute_flow -compile

report_timing \
    -setup \
    -npaths 10 \
    -detail full_path \
    -panel_name {Report Timing} \
    -multi_corner \
    -file "worst_case_paths.rpt"

project_close
