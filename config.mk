VERSAT_COMMON_FLAGS := -Wall -Wunused-variable -Wno-char-subscripts -Wno-switch-enum -Wno-switch -Wno-unused-function 

# Useful to run addressSanitizer often but unless we disable ASLR we are gonna have random segfaults for no reason. Enable every so often to check things out and disable again.
# VERSAT_COMMON_FLAGS += -fsanitize=address -static-libasan

#VERSAT_COMMON_FLAGS += -ggdb3 # Outputs more debug info but only when debuggin with gdb

# Use this flags to use compiler to fix Wall errors.
# Some errors are disabled since they are not problematic (like unused functions for operator== and stuff like that)
#VERSAT_COMMON_FLAGS += -Werror -Wno-char-subscripts -Wunused-function #-Wno-switch-enum

#VERSAT_COMMON_FLAGS += -Wall -Wno-switch-enum -Wno-switch -Wno-char-subscripts -Werror

VERSAT_DEBUG:=0

USE_FST_FORMAT ?= 0

