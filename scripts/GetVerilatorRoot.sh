#!/bin/bash
TEMP_DIR=$(mktemp -d)

pushd $TEMP_DIR &> /dev/null

echo "module Test(); endmodule" > Test.v
verilator --cc Test.v &> /dev/null

pushd ./obj_dir &> /dev/null
VERILATOR_ROOT=$(awk '"VERILATOR_ROOT" == $1 { print $3}' VTest.mk)
rm -r $TEMP_DIR

echo $VERILATOR_ROOT