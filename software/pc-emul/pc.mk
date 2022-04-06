#versat common parameters
include $(VERSAT_DIR)/software/software.mk

#pc sources

SRC+=$(VERSAT_PC_EMUL)/utils.c
SRC+=$(VERSAT_PC_EMUL)/template_engine.c
SRC+=$(VERSAT_PC_EMUL)/structParser.c
SRC+=$(VERSAT_PC_EMUL)/versatSpecificationParser.c
SRC+=$(VERSAT_SW_DIR)/versatCommon.c

HDR+=$(VERSAT_PC_EMUL)/template_engine.h
HDR+=versat_instance_template.h
HDR+=versat_instance_template_copy.v

OBJ+=./build/unitVerilogWrappers.o
OBJ+=./build/unitVCD.o
OBJ+=./build/versatCPP.o

./build/versatCPP.o: $(VERSAT_PC_EMUL)/versat.cpp
	mkdir -p ./build
	g++ -std=c++11 -c -o ./build/versatCPP.o -g -m32 $(VERSAT_PC_EMUL)/versat.cpp -I $(VERSAT_SW_DIR) -I ./build/

./build/unitVerilogWrappers.o: $(VERSAT_PC_EMUL)/unitVerilogWrappers.cpp $(UNIT_OBJ)
	mkdir -p ./build
	g++ -std=c++11 -c -o ./build/unitVerilogWrappers.o -g -m32 $(VERSAT_PC_EMUL)/unitVerilogWrappers.cpp -I $(VERSAT_SW_DIR) -I $(VERILATOR_INCLUDE) -I ./build/

./build/unitVCD.o: $(VERSAT_PC_EMUL)/unitVCD.cpp
	mkdir -p ./build
	g++ -std=c++11 -c -o ./build/unitVCD.o -g -m32 $(VERSAT_PC_EMUL)/unitVCD.cpp -I $(VERSAT_SW_DIR) -I $(VERILATOR_INCLUDE) -I ./build/

versat_instance_template.h: $(VERSAT_PC_EMUL)/versat_instance_template.v
	$(eval IN=$(VERSAT_PC_EMUL)/versat_instance_template.v)
	$(eval OUT=versat_instance_template.h)
	echo "#ifndef INCLUDED_VERSAT_INSTANCE_TEMPLATE_STR" > $(OUT)
	echo "#define INCLUDED_VERSAT_INSTANCE_TEMPLATE_STR" >> $(OUT)
	echo "static const char versat_instance_template_str[] = {" >> $(OUT)
	cat $(IN) | xxd -i >> $(OUT)
	echo ", 0x00};" >> $(OUT)
	echo "#endif // INCLUDED_VERSAT_INSTANCE_TEMPLATE_STR" >> $(OUT)

versat_instance_template_copy.v: $(VERSAT_PC_EMUL)/versat_instance_template.v
	cp $(VERSAT_PC_EMUL)/versat_instance_template.v versat_instance_template_copy.v

