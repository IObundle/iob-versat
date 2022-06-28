// Maybe better methods to embed data exist, but for now I just want a quick way of embedding data 

static unsigned int delayBuffer[] = {
   #{join "," for d delay} 
      @{d |> Hex}
   #{end}
};

struct HashKey{
   const char* key;
   int32_t index;
};

static HashKey instanceHashmap[] = {
   #{join "," for d instanceHashmap}
      #{if d.data == 0-1}
      {(const char*)0,-1}
      #{else}
      {"@{d.key}",@{d.data}}
      #{end}
   #{end}
};

static volatile int* delayBase = (volatile int*) @{versatBase + (nConfigs - nDelays) * 4 |> Hex};
static volatile int* configBase = (volatile int*) @{versatBase + 4 |> Hex};
static volatile int* stateBase = (volatile int*) @{versatBase + 4 |> Hex};
static volatile int* memMappedBase = (volatile int*) @{versatBase + memoryMappedBase * 4 |> Hex};

static FUInstance instancesBuffer[] = {
   #{debug 1} #{join "," for inst instances}
   {
      .name = @{inst.name |> GetHierarchyName |> String},
      #{if inst.declaration.isMemoryMapped} 
         .memMapped = (int*) @{versatBase + memoryMappedBase * 4 + inst.versatData.memoryAddressOffset * 4 |> Hex}, 
      #{else} 
         .memMapped = (int*) 0x0, 
      #{end}
      
      #{if inst.config + 0} 
         .config = (int*) @{inst.config - config + 4 + versatBase |> Hex}, 
      #{else} 
         .config = (int*) 0x0, 
      #{end}
      
      #{if inst.state + 0} 
         .state = (int*) @{inst.state - state + 4 + versatBase |> Hex} 
      #{else} 
         .state = (int*) 0x0
      #{end}
   }
   #{end}
   #{end}
};
