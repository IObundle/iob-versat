#versat common parameters
include $(VERSAT_DIR)/software/software.mk

#pc sources
HDR+=$(VERSAT_PC_EMUL)/templateEngine.hpp

CPP_FILES = $(wildcard $(VERSAT_PC_EMUL)/*.cpp)
CPP_OBJ = $(patsubst $(VERSAT_PC_EMUL)/%.cpp,./build/%.o,$(CPP_FILES))

./build/typeInfo.inc: $(UNIT_HDR) $(VERSAT_HDR)
	make -C $(VERSAT_SW_DIR)/tools/ tools
	$(VERSAT_SW_DIR)/tools/structParser.out ./build/typeInfo.inc $(VERSAT_SW_DIR)/versat.hpp $(VERSAT_SW_DIR)/utils.hpp 

./build/%.o: $(VERSAT_PC_EMUL)/%.cpp $(UNIT_HDR) $(VERSAT_HDR) ./build/typeInfo.inc
	mkdir -p ./build
	$(info $@)
	g++ -std=c++11 -c -o $@ -g -m32 $< -I $(VERSAT_SW_DIR) -I $(VERILATOR_INCLUDE) -I ./build/
