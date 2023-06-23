// This file has been auto-generated

#ifndef INCLUDED_VERSAT_ACCELERATOR_HEADER
#define INCLUDED_VERSAT_ACCELERATOR_HEADER

struct AcceleratorConfig{
#{for wire orderedConfigs.configs}
   int @{wire.name};
#{end}
#{for wire orderedConfigs.statics}
   int @{wire.name};
#{end}
#{for wire orderedConfigs.delays}
   int @{wire.name};
#{end}
};

struct AcceleratorState{
#{for pair namedStates}
#{set name pair.first} #{set conf pair.second}
int @{name};
#{end}
};

static const int memMappedStart = @{memoryMappedBase * 4 |> Hex};

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
void RunAccelerator(int times);
void VersatMemoryCopy(int* dest,int* data,int size);
void VersatUnitWrite(int addr,int val);

// Needed by PC-EMUL to correctly simulate the design, embedded compiler should remove these symbols from firmware because not used by them 
static const char* acceleratorTypeName = "@{accelType}";
static bool isSimpleAccelerator = @{isSimple};

static const int staticStart = @{nConfigs * 4 |> Hex};
static const int delayStart = @{(nConfigs + nStatics) * 4 |> Hex};
static const int configStart = @{versatConfig * 4 |> Hex};
static const int stateStart = @{versatState * 4 |> Hex};

extern volatile AcceleratorConfig* accelConfig;
extern volatile AcceleratorState* accelState;

#{if isSimple}
// Simple input and output connection for simple accelerators
#define SimpleInputStart ((int*) accelConfig)
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


#endif // INCLUDED_VERSAT_ACCELERATOR_HEADER
