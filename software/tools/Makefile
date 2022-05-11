VERSAT_DIR=../..

#versat common parameters
include $(VERSAT_DIR)/software/pc-emul/pc.mk

SRC += $(VERSAT_DIR)/software/pc-emul/parser.cpp
SRC += $(VERSAT_DIR)/software/pc-emul/utils.cpp

$(VERSAT_SW_DIR)/tools/structParser.out: $(VERSAT_SW_DIR)/tools/structParser.cpp $(SRC)
	g++ -std=c++11 -o $@ -g -m32 $< -I $(VERSAT_DIR)/software/ -I $(VERSAT_DIR)/software/pc-emul/ $(SRC)

tools: $(VERSAT_SW_DIR)/tools/structParser.out

