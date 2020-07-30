VERSAT_HW_DIR:=$(VERSAT_DIR)/hardware

#include
VERSAT_INC_DIR:=$(VERSAT_HW_DIR)/include/common
INCLUDE+=$(incdir) $(VERSAT_INC_DIR)
#Check for top level versat.json
ifeq (,$(wildcard $(FIRM_DIR)/xversat.json))
INCLUDE+=$(incdir) $(VERSAT_INC_DIR)/versat
endif

#headers
VHDR+=$(wildcard $(VERSAT_INC_DIR)/*.vh)

#sources
VERSAT_SRC_DIR:=$(VERSAT_HW_DIR)/src
#VSRC+=$(wildcard $(VERSAT_SRC_DIR)/*.v)
VSRC+=$(VERSAT_SRC_DIR)/xaddrgen.v \
	$(VERSAT_SRC_DIR)/xaddrgen2.v \
	$(VERSAT_SRC_DIR)/xalu.v \
	$(VERSAT_SRC_DIR)/xalulite.v \
	$(VERSAT_SRC_DIR)/xinmux.v \
	$(VERSAT_SRC_DIR)/xmuladd.v \
	$(VERSAT_SRC_DIR)/xversat.v \
	$(VERSAT_SRC_DIR)/xbs.v \
	$(VERSAT_SRC_DIR)/xclz.v \
	$(VERSAT_SRC_DIR)/xconf.v \
	$(VERSAT_SRC_DIR)/xconf_mem.v \
	$(VERSAT_SRC_DIR)/xmul.v \
	$(VERSAT_SRC_DIR)/xstage.v
