include $(VERSAT_DIR)/config.mk

VERSAT_REQUIRE_TYPE:= type verilogParsing templateEngine

VERSAT_COMMON_SRC_NO_TYPE := $(filter-out $(patsubst %,$(VERSAT_COMMON_DIR)/%.cpp,$(VERSAT_REQUIRE_TYPE)),$(wildcard $(VERSAT_COMMON_DIR)/*.cpp))
VERSAT_COMMON_HDR_NO_TYPE := $(filter-out $(patsubst %,$(VERSAT_COMMON_DIR)/%.hpp,$(VERSAT_REQUIRE_TYPE)),$(wildcard $(VERSAT_COMMON_DIR)/*.hpp))
VERSAT_COMMON_OBJ_NO_TYPE := $(patsubst $(VERSAT_COMMON_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(VERSAT_COMMON_SRC_NO_TYPE))

VERSAT_COMMON_SRC:=$(VERSAT_COMMON_SRC_NO_TYPE) $(patsubst %,$(VERSAT_COMMON_DIR)/%.cpp,$(VERSAT_REQUIRE_TYPE))
VERSAT_COMMON_SRC+=$(BUILD_DIR)/repr.cpp
VERSAT_COMMON_HDR:=$(VERSAT_COMMON_HDR_NO_TYPE) $(patsubst %,$(VERSAT_COMMON_DIR)/%.hpp,$(VERSAT_REQUIRE_TYPE))
VERSAT_COMMON_SRC+=$(BUILD_DIR)/repr.hpp
VERSAT_COMMON_OBJ:=$(VERSAT_COMMON_OBJ_NO_TYPE) $(patsubst %,$(BUILD_DIR)/%.o,$(VERSAT_REQUIRE_TYPE))
VERSAT_COMMON_SRC+=$(BUILD_DIR)/repr.o

#VERSAT_COMMON_SRC:=$(abspath $(VERSAT_COMMON_SRC))

VERSAT_COMMON_INCLUDE := -I$(VERSAT_COMMON_DIR) -I$(VERSAT_SW_DIR)

# After first rule, only build needed objects
VERSAT_COMMON_DEPENDS := $(patsubst %.o,%.d,$(VERSAT_COMMON_OBJ))
-include  $(VERSAT_COMMON_DEPENDS)

# -rdynamic - used to allow backtraces

$(BUILD_DIR)/%.o : $(VERSAT_COMMON_DIR)/%.cpp
	-g++ $(VERSAT_DEFINE) -MMD -rdynamic $(VERSAT_COMMON_FLAGS) -std=c++17 -c -o $@ -DROOT_PATH=\"$(abspath ../)\" -DVERSAT_DEBUG $(GLOBAL_CFLAGS) $< $(VERSAT_COMMON_INCLUDE)
