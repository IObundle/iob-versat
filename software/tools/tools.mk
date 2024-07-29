include $(VERSAT_DIR)/config.mk
include $(VERSAT_COMMON_DIR)/common.mk
include $(VERSAT_TEMPLATE_DIR)/templates.mk

TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/utils.hpp
TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/utilsCore.hpp
TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/memory.hpp
TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/templateEngine.hpp
TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/parser.hpp
TYPE_INFO_HDR += $(VERSAT_COMMON_DIR)/verilogParsing.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/versat.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/declaration.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/accelerator.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/configurations.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/codeGeneration.hpp
TYPE_INFO_HDR += $(VERSAT_COMPILER_DIR)/versatSpecificationParser.hpp

VERSAT_INCLUDE += -I$(VERSAT_PC_DIR) -I$(VERSAT_COMPILER_DIR) -I$(BUILD_DIR)/ -I$(VERSAT_COMMON_DIR)

$(BUILD_DIR)/embedFile: $(VERSAT_TOOLS_DIR)/embedFile.cpp $(VERSAT_COMMON_OBJ_NO_TYPE)
	-g++ -DPC -std=c++17 -MMD -MP -DVERSAT_DEBUG -DSTANDALONE -o $@ $(VERSAT_COMMON_FLAGS) $(VERSAT_COMMON_INCLUDE) $< $(VERSAT_COMMON_OBJ_NO_TYPE)

type-info: $(TYPE_INFO_HDR)
	python3 $(VERSAT_TOOLS_DIR)/structParser.py $(VERSAT_DIR)/software $(TYPE_INFO_HDR) || true

$(VERSAT_DIR)/software/autoRepr.cpp $(VERSAT_DIR)/software/typeInfo.cpp: $(TYPE_INFO_HDR)
	python3 $(VERSAT_TOOLS_DIR)/structParser.py $(VERSAT_DIR)/software $(TYPE_INFO_HDR) || true

$(BUILD_DIR)/typeInfo.o: $(VERSAT_DIR)/software/typeInfo.cpp
	-g++ -DPC -MMD -MP -std=c++17 $(VERSAT_COMMON_FLAGS) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)

$(BUILD_DIR)/autoRepr.o: $(VERSAT_DIR)/software/autoRepr.cpp
	-g++ -DPC -MMD -MP -std=c++17 $(VERSAT_COMMON_FLAGS) -c -o $@ $(GLOBAL_CFLAGS) $< $(VERSAT_INCLUDE)
