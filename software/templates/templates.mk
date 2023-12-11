include $(VERSAT_DIR)/config.mk
include $(VERSAT_COMMON_DIR)/common.mk

VERSAT_TEMPLATES:=$(wildcard $(VERSAT_TEMPLATE_DIR)/*.tpl)
VERSAT_TEMPLATES_OBJ:=$(BUILD_DIR)/templateData.o

$(BUILD_DIR)/templateData.hpp $(BUILD_DIR)/templateData.cpp &: $(VERSAT_TEMPLATES) $(BUILD_DIR)/embedFile
	$(BUILD_DIR)/embedFile $(BUILD_DIR)/templateData $(VERSAT_TEMPLATES)

$(BUILD_DIR)/templateData.o: $(BUILD_DIR)/templateData.cpp $(BUILD_DIR)/templateData.hpp
	-g++ -DPC -std=c++17 -MMD -MP -DVERSAT_DEBUG -DSTANDALONE -c -o $@ -g $(VERSAT_COMMON_INCLUDE) $<
