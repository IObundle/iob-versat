// This file has been auto-generated

#ifndef INCLUDED_VERSAT_ACCELERATOR_HEADER
#define INCLUDED_VERSAT_ACCELERATOR_HEADER

#ifdef __cplusplus
#include <cstdint>
#else
#include "stdbool.h"
#include "stdint.h"
#endif

typedef intptr_t iptr;

// Config

#{for type nonMergedStructures}
typedef struct {
#{for entry type.entries}
#{if entry.typeAndNames.size > 1}
union{
#{for typeAndName entry.typeAndNames}
@{typeAndName.type} @{typeAndName.name};
#{end}
};
#{else}
@{entry.typeAndNames[0].type} @{entry.typeAndNames[0].name};
#{end}
#{end}
} @{type.name}Config;
#define VERSAT_DEFINED_@{type.name}
#{end}

// Address

#{for type addressStructures}
typedef struct {
#{for entry type.entries}
  @{entry.typeAndNames[0].type} @{entry.typeAndNames[0].name};
#{end}
} @{type.name}Addr;
#define VERSAT_DEFINED_@{type.name}Addr
#{end}

typedef struct{
#{for wire orderedConfigs}
  iptr @{wire.name};
#{end}
} AcceleratorConfig;

typedef struct{
#{for name namedStates}
  int @{name};
#{end}
} AcceleratorState;

static const int memMappedStart = @{memoryMappedBase |> Hex};
static const int versatAddressSpace = 2 * @{memoryMappedBase |> Hex};

extern iptr versat_base;

// NOTE: The address for memory mapped units depends on the address of
//       the accelerator.  Because of this, the full address can only
//       be calculated after calling versat_init(iptr), which is the
//       function that sets the versat_base variable.  It is for this
//       reason that the address info for every unit is a define. Addr
//       variables must be instantiated only after calling
//       versat_base.

// Base address for each memory mapped unit
#{for pair namedMem}
#define @{pair.first} (versat_base + memMappedStart + @{pair.second |> Hex})
#{end}

#define ACCELERATOR_TOP_ADDR_INIT {#{join "," pair namedMem} (void*) @{pair.first} #{end}}

static unsigned int delayBuffer[] = {#{join "," d delay} @{d |> Hex} #{end}};

#ifdef __cplusplus
extern "C" {
#endif

// Always call first before calling any other function.
void versat_init(int base);

// In pc-emul provides a low bound on performance.
// In sim-run refines the lower bound but still likely to be smaller than reality due to memory delays that are only present in real circuits.
int GetAcceleratorCyclesElapsed();

void RunAccelerator(int times);
void StartAccelerator();
void EndAccelerator();
void VersatMemoryCopy(void* dest,void* data,int size);
void VersatUnitWrite(int baseaddr,int index,int val);
int VersatUnitRead(int baseaddr,int index);
float VersatUnitReadFloat(int base,int index);
void SignalLoop();

// PC-Emul side functions that allow to enable or disable certain portions of the emulation
// Their embedded counterparts simply do nothing
void ConfigCreateVCD(bool value);
void ConfigSimulateDatabus(bool value); 

#ifdef __cplusplus
} // extern "C"
#endif

// Needed by PC-EMUL to correctly simulate the design, embedded compiler should remove these symbols from firmware because not used by them 
static const char* acceleratorTypeName = "@{accelName}";
static bool isSimpleAccelerator = @{isSimple};
static bool acceleratorSupportsDMA = @{useDMA};

static const int staticStart = @{nConfigs |> Hex} * sizeof(iptr);
static const int delayStart = @{(nConfigs + nStatics) |> Hex} * sizeof(iptr);
static const int configStart = @{versatConfig |> Hex} * sizeof(iptr);
static const int stateStart = @{versatState |> Hex} * sizeof(int);

extern volatile AcceleratorConfig* accelConfig;
extern volatile AcceleratorState* accelState;

#{if isSimple}
// Simple input and output connection for simple accelerators
#define SimpleInputStart ((iptr*) accelConfig)
#define SimpleOutputStart ((int*) accelState)
#{end}

#{for wire orderedConfigs}
#define ACCEL_@{wire.name} accelConfig->@{wire.name}
#{end}

#{for name namedStates}
#define ACCEL_@{name} accelState->@{name}
#{end}

#{if doingMerged}
typedef enum{
   #{join "," info mergeInfo}
        @{info.name} = @{index}
   #{end}  
} MergeType;

#ifdef __cplusplus
extern "C" {
#endif

static inline void ActivateMergedAccelerator(MergeType type){
   ACCEL_@{accelName}_versat_merge_mux_sel = (int) type;
}

#ifdef __cplusplus
}
#endif

#{end}

#endif // INCLUDED_VERSAT_ACCELERATOR_HEADER

