VERSAT_SW_DIR:=$(VERSAT_DIR)/software

#Python directory
VERSAT_PYTHON_DIR:=$(VERSAT_SW_DIR)/python

#VHeader directory
ifeq ($(VERSAT_INC_DIR),)
VERSAT_INC_DIR:=$(VERSAT_DIR)/hardware/include/common
endif
