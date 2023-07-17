printf "include Vversat_test_verilator.mk\nBUILD_DIR:=$1\nALL_O:=\$(addsuffix .o,\$(VM_GLOBAL_FAST) \$(VM_GLOBAL_SLOW))\nALL_C:=\$(patsubst %%.o,%%.cpp,\$(ALL_O))\nOUT:=\$(patsubst %%.o,\$(BUILD_DIR)/%%.o,\$(ALL_O))\n\$(BUILD_DIR)/%%.o: %%.cpp\n\t@g++ -g -w -c -o \$@ \$< -I\$(VERILATOR_ROOT)/include\nmyCode: \$(OUT) Makefile\n\t@echo \$(OUT)\n" > /tmp/versat_makefile2.mk
make --no-print-directory -I$1/obj_dir -f /tmp/versat_makefile2.mk myCode