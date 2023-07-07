VERSAT_DIR:=../..

VERSAT_DEFINE=-DPC

include $(VERSAT_DIR)/core.mk
include $(VERSAT_TOOLS_DIR)/tools.mk

structParser: $(BUILD_DIR)/structParser 
verilogParser: $(BUILD_DIR)/verilogParser
embedFile: $(BUILD_DIR)/embedFile

all: structParser verilogParser embedFile

.PHONY: structParser verilogParser embedFile
