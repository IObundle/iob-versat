// This file has been auto-generated

#ifndef INCLUDED_VERSAT_ACCELERATOR_HEADER
#define INCLUDED_VERSAT_ACCELERATOR_HEADER

#include <stdint.h>
typedef intptr_t iptr;
#define bool char
#define true 1
#define false 0

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

typedef struct{
#{for pair namedStates}
#{set name pair.first} #{set conf pair.second}
int @{name};
#{end}
} AcceleratorState;

static const int memMappedStart = @{memoryMappedBase * 4 |> Hex};

extern int versat_base;

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
void VersatMemoryCopy(iptr* dest,iptr* data,int size);
void VersatUnitWrite(int addr,int val);
int VersatUnitRead(int base,int index);
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

#{for pair namedStates}
#{set name pair.first} #{set conf pair.second}
#define ACCEL_@{name} accelState->@{name}
#{end}

#undef bool
#undef true
#undef false

#endif // INCLUDED_VERSAT_ACCELERATOR_HEADER

