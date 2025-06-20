VERSAT_COMMON_FLAGS := -Wall -Wswitch-enum # -Werror=switch-enum

VERSAT_COMMON_FLAGS += -ggdb3 # Outputs more debug info but only when debuggin with gdb

# Use this flags to use compiler to fix Wall errors.
# Some errors are disabled since they are not problematic (like unused functions for operator== and stuff like that)
# VERSAT_COMMON_FLAGS += -Werror -Wall -Wno-char-subscripts #-Wno-unused-function -Wno-switch-enum

VERSAT_DEBUG:=0

USE_FST_FORMAT ?= 0

