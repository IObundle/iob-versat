#versat common parameters
include $(VERSAT_DIR)/software/software.mk

#pc sources
SRC+=$(VERSAT_PC_EMUL)/versat.c

HDR+=$(VERSAT_PC_EMUL)/versat_instance_template.h

OBJ+=$(VERSAT_SW_DIR)/build/unitVerilogWrappers.o

$(VERSAT_SW_DIR)/build/unitVerilogWrappers.o: $(VERSAT_PC_EMUL)/unitVerilogWrappers.cpp
	g++ -c -o $(VERSAT_SW_DIR)/build/unitVerilogWrappers.o $(VERSAT_PC_EMUL)/unitVerilogWrappers.cpp -I $(VERSAT_SW_DIR) -I $(VERILATOR_INCLUDE)

$(VERSAT_PC_EMUL)/versat_instance_template.h: $(VERSAT_PC_EMUL)/versat_instance_template.v
	$(eval IN=$(VERSAT_PC_EMUL)/versat_instance_template.v)
	$(eval OUT=$(VERSAT_PC_EMUL)/versat_instance_template.h)
	echo "#ifndef INCLUDED_VERSAT_INSTANCE_TEMPLATE_STR" > $(OUT)
	echo "#define INCLUDED_VERSAT_INSTANCE_TEMPLATE_STR" >> $(OUT)
	echo "static const char versat_instance_template_str[] = {" >> $(OUT)
	cat $(IN) | xxd -i >> $(OUT)
	echo ", 0x00};" >> $(OUT)
	echo "#endif // INCLUDED_VERSAT_INSTANCE_TEMPLATE_STR" >> $(OUT)