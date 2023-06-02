VERSAT_DIR:=../..

include ./pc-emul.mk

$(VERSAT_DIR)/versatCompiler: $(CPP_OBJ) $(VERSAT_SW_DIR)/pc-emul/tools/versatCompiler.cpp
	g++ -DPC -std=c++17 -DVERSAT_DEBUG -DSTANDALONE -o $@ -g $(VERSAT_SW_DIR)/pc-emul/tools/versatCompiler.cpp $(VERSAT_DIR)/registerUnitsStub.cpp $(VERSAT_INCLUDE) ./build/*.o -lstdc++ -lm -lgcc -lc -pthread -ldl #-lSDL2  -lGL -ldl

compiler: $(VERSAT_DIR)/versatCompiler

