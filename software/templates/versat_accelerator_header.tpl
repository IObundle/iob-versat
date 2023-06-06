// This file has been auto-generated

#ifndef INCLUDED_VERSAT_ACCELERATOR_HEADER
#define INCLUDED_VERSAT_ACCELERATOR_HEADER

struct AcceleratorConfig{
#{for pair namedConfigs}
#{set name pair.first} #{set conf pair.second}
int @{name};
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

// Needed by PC-EMUL to correctly simulate the design, embedded compiler should remove these symbols from firmware because not used by them 
static const char* acceleratorTypeName = "@{accelType}";
static bool isSimpleAccelerator = @{isSimple};

static const int staticStart = @{nConfigs * 4 |> Hex};
static const int delayStart = @{(nConfigs + nStatics) * 4 |> Hex};
static const int configStart = @{versatConfig * 4 |> Hex};
static const int stateStart = @{versatState * 4 |> Hex};

extern volatile AcceleratorConfig* accelConfig;
extern volatile AcceleratorState* accelState;

#{for pair namedConfigs}
#{set name pair.first} #{set conf pair.second}
#define @{name} accelConfig->@{name}
#{end}

#{for pair namedStates}
#{set name pair.first} #{set conf pair.second}
#define @{name} accelState->@{name}
#{end}


#endif // INCLUDED_VERSAT_ACCELERATOR_HEADER
