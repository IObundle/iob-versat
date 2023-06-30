VERILATE_UNIT_BASIC += Buffer FixedBuffer SwapEndian LookupTable Mul Generator IntSqrt LookupTableRead StridedMerge
VERILATE_UNIT_BASIC += Reg Mem Muladd VRead VWrite Mux2 CombMux2 CombMux4 Merge Const TestConst TimedMux
VERILATE_UNIT_BASIC += FloatAdd FloatSub FloatMul FloatDiv FloatSqrt Q16Sqrt DoneCycle ChangeDetect Store
VERILATE_UNIT_BASIC += FloatMax FloatMin Float2Int Float2UInt Int2Float SignalMemStorage SignalAccum Equality

SHARED_HARDWARE := $(patsubst %,$(VERSAT_DIR)/hardware/src/%.v,$(VERILATE_UNIT_BASIC))

V_INCLUDE+=-I$(VERSAT_DIR)/submodules/MEM/hardware/ram/tdp_ram
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/MEM/hardware/ram/2p_ram
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/MEM/hardware/ram/dp_ram
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/MEM/hardware/fifo/sfifo
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/MEM/hardware/fifo
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/FPU/hardware/include
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/FPU/hardware/src
V_INCLUDE+=-I$(VERSAT_DIR)/submodules/FPU/submodules/DIV/hardware/src

VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/ram/tdp_ram/iob_tdp_ram.v # used by xmem
VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/ram/2p_ram/iob_2p_ram.v # used by vread and vwrite
VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/ram/dp_ram/iob_dp_ram.v # Lookup table
VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/fifo/sfifo/iob_sync_fifo.v
VSRC+=$(VERSAT_DIR)/submodules/MEM/hardware/fifo/bin_counter.v
VSRC+=$(VERSAT_DIR)/submodules/FPU/submodules/DIV/hardware/src/div_subshift.v
VSRC+=$(wildcard $(VERSAT_DIR)/submodules/FPU/hardware/src/*.v)

VERSAT_VSRC+=$(VERSAT_DIR)/hardware/src/MyAddressGen.v 
VERSAT_VSRC+=$(VERSAT_DIR)/hardware/src/xaddrgen.v 
VERSAT_VSRC+=$(VERSAT_DIR)/hardware/src/xaddrgen2.v 
VERSAT_VSRC+=$(VERSAT_DIR)/hardware/src/MemoryWriter.v
VERSAT_VSRC+=$(VERSAT_DIR)/hardware/src/MemoryReader.v
