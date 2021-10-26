#!/bin/bash
source /opt/ic_tools/init/init-xcelium1903-hf013
xmvlog $CFLAGS $VSRC
xmelab $EFLAGS worklib."$CMDGOALS"_tb:module
xmsim  $SFLAGS worklib."$CMDGOALS"_tb:module
