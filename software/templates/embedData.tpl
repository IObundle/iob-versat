// Maybe better methods to embed data exist, but for now I just want a quick way of embedding data 

unsigned int delayBuffer[] = {
   #{join "," for d delay} 
      @{d |> Hex}
   #{end}
};

unsigned int staticBuffer[] = {
   #{join "," for d staticBuffer} 
      @{d |> Hex}
   #{end} 
};

static volatile int* staticBase = (volatile int*) @{versatBase + nConfigs * 4 |> Hex};
static volatile int* delayBase = (volatile int*) @{versatBase + (nConfigs + nStatics) * 4 |> Hex};
static volatile int* configBase = (volatile int*) @{versatBase + versatConfig * 4 |> Hex};
static volatile int* stateBase = (volatile int*) @{versatBase + versatState * 4 |> Hex};
static volatile int* memMappedBase = (volatile int*) @{versatBase + memoryMappedBase * 4 |> Hex};

#{set delaySeen 0}
FUInstance instancesBuffer[] = {
   #{join "," for inst instances}
   { // @{index}
      .name = "@{inst.name |> Identify}",
      #{if inst.declaration.isMemoryMapped}
         .memMapped = (int*) @{versatBase + memoryMappedBase * 4 + inst.memMapped * 4 |> Hex},
      #{else} 
         .memMapped = (int*) 0x0, 
      #{end}
      
      #{if inst.config} 
           .config = (int*) @{inst.config - config + versatConfig * 4 + versatBase |> Hex}, 
      #{else} 
           .config = (int*) 0x0, 
      #{end}
      
      #{if inst.state} 
         .state = (int*) @{inst.state - state + versatState * 4 + versatBase |> Hex},
      #{else} 
         .state = (int*) 0x0,
      #{end}

      #{if inst.declaration.delayOffsets.max}
         .delay = (int*) 0x0, // &delayBase[@{delaySeen}], - not working because composite accelerator is double adding delay 
      #{set delaySeen delaySeen + inst.declaration.delayOffsets.max}
      #{else}
         .delay = (int*) 0x0,
      #{end}

      .numberChilds = @{inst.declaration.fixedDelayCircuit |> CountNonOperationChilds}
   }
   #{end}
};

#{if IsSimple}
static int simpleInputs = @{simpleInputs};
static int simpleOutputs = @{simpleOutputs};
static int inputStart = @{inputStart};
static int outputStart = @{outputStart};
#{else}
static int simpleInputs = 0;
static int simpleOutputs = 0;
static int inputStart = 0;
static int outputStart = 0;
#{end}
