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

#{for type structures}
typedef struct {
#{for entry type.entries}
@{entry.type} @{entry.name};
#{end}
} @{type.name}Config;

#{end}

typedef struct{
#{for wire orderedConfigs.configs}
   iptr @{wire.name};
#{end}
#{for wire orderedConfigs.statics}
   iptr @{wire.name};
#{end}
#{for wire orderedConfigs.delays}
   iptr @{wire.name};
#{end}
} AcceleratorConfig;

#{if doingMerged}
#{for arr mergedConfigs}
#{set i index}
typedef struct{
#{for str arr}
  iptr @{str};
#{end}
} Merged@{i};
#{end}
#{end}

typedef struct{
#{for pair namedStates}
#{set name pair.first} #{set conf pair.second}
int @{name};
#{end}
} AcceleratorState;

static const int memMappedStart = @{memoryMappedBase |> Hex};
static const int versatAddressSpace = 2 * @{memoryMappedBase |> Hex};

extern int versat_base;

// Base address for each memory mapped unit
#{for pair namedMem}
#define @{pair.first} (versat_base + memMappedStart + @{pair.second.ptr})
#{end}

static unsigned int delayBuffer[] = {
   #{join "," for d delay} 
      @{d |> Hex}
   #{end}
};

static unsigned int staticBuffer[] = {
   #{join "," for d staticBuffer} 
      @{d |> Hex}
   #{end} 
};

void versat_init(int base);

void Debug();
void RunAccelerator(int times);
void StartAccelerator();
void EndAccelerator();
void VersatMemoryCopy(void* dest,void* data,int size);
void VersatUnitWrite(int baseaddr,int index,int val);
int VersatUnitRead(int baseaddr,int index);
float VersatUnitReadFloat(int base,int index);
void SignalLoop();

// PC-Emul side functions that allow to enable or disable certain portions of the emulation
#ifdef PC
void ConfigCreateVCD(bool value);
void ConfigSimulateDatabus(bool value); 
#else
#define ConfigCreateVCD(...) ((void)0)
#define ConfigSimulateDatabus(...) ((void)0)
#endif

// Needed by PC-EMUL to correctly simulate the design, embedded compiler should remove these symbols from firmware because not used by them 
static const char* acceleratorTypeName = "@{accelType}";
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

#{for wire orderedConfigs.configs}
#define ACCEL_@{wire.name} accelConfig->@{wire.name}
#{end}
#{for wire orderedConfigs.statics}
#define ACCEL_@{wire.name} accelConfig->@{wire.name}
#{end}
#{for wire orderedConfigs.delays}
#define ACCEL_@{wire.name} accelConfig->@{wire.name}
#{end}

#{if doingMerged}
#{for arr mergedConfigs}
#{set i index}
#{for str arr}
#define MERGED_@{i}_@{str} ((Merged@{i}*) accelConfig)->@{str}
#{end}
#{end}
#{end}

#{for pair namedStates}
#{set name pair.first} #{set conf pair.second}
#define ACCEL_@{name} accelState->@{name}
#{end}

#endif // INCLUDED_VERSAT_ACCELERATOR_HEADER

