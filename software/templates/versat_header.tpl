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

#{for type configStructures}
#define VERSAT_DEFINED_@{type.name}
typedef struct {
#{for entry type.entries}
#{if entry.typeAndNames.size > 1}
  union{
#{for typeAndName entry.typeAndNames}
#{if typeAndName.arraySize > 1}
  @{typeAndName.type} @{typeAndName.name}[@{typeAndName.arraySize}];
#{else}
  @{typeAndName.type} @{typeAndName.name};
#{end}
#{end}
};
#{else}
#{if entry.typeAndNames[0].arraySize > 1}
  @{entry.typeAndNames[0].type} @{entry.typeAndNames[0].name}[@{entry.typeAndNames[0].arraySize}];
#{else}
  @{entry.typeAndNames[0].type} @{entry.typeAndNames[0].name};
#{end}
#{end}
#{end}
} @{type.name}Config;
#{end}

typedef struct{
#{for name namedStates}
  int @{name};
#{end}
} @{typeName}State;

// Address

#{for type addressStructures}
#define VERSAT_DEFINED_@{type.name}Addr
typedef struct {
#{for entry type.entries}
  @{entry.typeAndNames[0].type} @{entry.typeAndNames[0].name};
#{end}
} @{type.name}Addr;

#{end}

typedef struct{
#{for elem structuredConfigs}
#{if elem.typeAndNames.size > 1}
  union{
#{for typeAndName elem.typeAndNames}
    @{typeAndName.type} @{typeAndName.name};
#{end}
  };
#{else}
  @{elem.typeAndNames[0].type} @{elem.typeAndNames[0].name};
#{end}
#{end}
} AcceleratorConfig;

typedef struct {
#{for elem allStatics}
  iptr @{elem.name};
#{end}
} AcceleratorStatic;

typedef struct {
  union{
    struct{
#{for i delays}
      iptr TOP_Delay@{i};
#{end}
    };
    iptr delays[@{delays}]; 
  };
} AcceleratorDelay;

static const int memMappedStart = @{memoryMappedBase |> Hex};
static const int versatAddressSpace = 2 * @{memoryMappedBase |> Hex};

extern iptr versat_base;

// NOTE: The address for memory mapped units depends on the address of
//       the accelerator.  Because of this, the full address can only
//       be calculated after calling versat_init(iptr), which is the
//       function that sets the versat_base variable.  It is for this
//       reason that the address info for every unit is a define. Addr
//       variables must be instantiated only after calling
//       versat_init.

// Base address for each memory mapped unit
#{for pair namedMem}
#define @{pair.first} ((void*) (versat_base + memMappedStart + @{pair.second |> Hex}))
#{end}

#define ACCELERATOR_TOP_ADDR_INIT {#{join "," pair namedMem} @{pair.first} #{end}}

static unsigned int delayBuffer[] = {#{join "," d delay} @{d |> Hex} #{end}};
static AcceleratorDelay accelDelay = {#{join "," d delay} @{d |> Hex} #{end}};

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
void VersatMemoryCopy(void* dest,const void* data,int size);
void VersatUnitWrite(const void* baseaddr,int index,int val);
int VersatUnitRead(const void* baseaddr,int index);
float VersatUnitReadFloat(const void* baseaddr,int index);
void SignalLoop();
void VersatLoadDelay(const unsigned int* delayBuffer);

// PC-Emul side functions that allow to enable or disable certain portions of the emulation
// Their embedded counterparts simply do nothing
void ConfigEnableDMA(bool value);
void ConfigCreateVCD(bool value);
void ConfigSimulateDatabus(bool value); 

#ifdef __cplusplus
} // extern "C"
#endif

// Needed by PC-EMUL to correctly simulate the design, embedded compiler should remove these symbols from firmware because not used by them 
static const char* acceleratorTypeName = "@{typeName}";
static bool isSimpleAccelerator = @{isSimple};
static bool acceleratorSupportsDMA = @{useDMA};

static const int staticStart = @{nConfigs |> Hex} * sizeof(iptr);
static const int delayStart = @{(nConfigs + nStatics) |> Hex} * sizeof(iptr);
static const int configStart = @{versatConfig |> Hex} * sizeof(iptr);
static const int stateStart = @{versatState |> Hex} * sizeof(int);

#{if configStructures.size > 0}
static const unsigned int AcceleratorConfigSize = sizeof(@{typeName}Config);

extern volatile @{typeName}Config* accelConfig; // @{nConfigs}
#{end}

#{if nStates > 0}
extern volatile @{typeName}State* accelState; // @{nStates}
#{end}

extern volatile AcceleratorStatic* accelStatic;

#{for elem allStatics}
#define ACCEL_@{elem.name} accelStatic->@{elem.name}
#{end}

#{if isSimple}
// Simple input and output connection for simple accelerators
#define NumberSimpleInputs @{simpleInputs}
#define NumberSimpleOutputs @{simpleOutputs}
#define SimpleInputStart ((iptr*) accelConfig)
#define SimpleOutputStart ((int*) accelState)
#{end}

#{if mergeNames.size > 1}

#{for delayArray mergedDelays}
static unsigned int delayBuffer_@{index}[] = {#{join "," d delayArray} @{d |> Hex} #{end}};
#{end}

static unsigned int* delayBuffers[] = {#{join "," name mergeNames}delayBuffer_@{index}#{end}};

typedef enum{
   #{join "," name mergeNames} MergeType_@{name} = @{index} #{end}  
} MergeType;

#ifdef __cplusplus
extern "C" {
#endif

#{if outputChangeDelay}
static inline void ChangeDelay(int oldDelay,int newDelay){
  if(oldDelay == newDelay){
    return;
  }

  volatile int* delayBase = (volatile int*) (versat_base + delayStart);

  switch(oldDelay){
#{for amount amountMerged}
    case @{amount}:{
      switch(newDelay){
  #{for diff differences}
    #{if diff.oldIndex == amount}
        case @{diff.newIndex}: {
          #{for difference diff.differences}
          delayBase[@{difference.index}] = @{difference.newValue |> Hex};
          #{end}
        } break;
    #{end}
  #{end}
      }
    } break;
#{end}
  }
}
#{end}

static inline void ActivateMergedAccelerator(MergeType type){
   static int lastLoaded = -1;
   int asInt = (int) type;
   
   if(lastLoaded == asInt){
     return;
   }
   lastLoaded = asInt;

   switch(type){
#{for i mergeNames.size}
   case MergeType_@{mergeNames[i]}: {
   #{for muxInfo mergeMux[i]}
     accelConfig->@{muxInfo.name}.sel = @{muxInfo.val};
   #{end}
   } break;
#{end}
   }

   VersatLoadDelay(delayBuffers[asInt]);
}

#ifdef __cplusplus
}
#endif

#{end}

#{for a addrGen}
@{a}
#{end}

#endif // INCLUDED_VERSAT_ACCELERATOR_HEADER

