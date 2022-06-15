// Maybe better methods to embed data exist, but for now I just want a quick way of embedding data 

static unsigned int delayBuffer[] = {
   #{join "," for d delay} 
      @{d |> Hex}
   #{end}
};

static volatile int* delayBase = (volatile int*) @{versatBase + nConfigs * 4 + 4 |> Hex};
static volatile int* configBase = (volatile int*) @{versatBase + 4 |> Hex};
static volatile int* stateBase = (volatile int*) @{versatBase + 4 |> Hex};
static volatile int* memMappedBase = (volatile int*) @{versatBase + 2 ** memoryConfigDecisionBit |> Hex};

static FUInstance instancesBuffer[] = {
   #{join "," for inst instances}
   {
      .name = @{inst.name |> GetHierarchyName |> String},
      #{if inst.memMapped + 0} 
         .memMapped = (int*) @{inst.memMapped - memMapped + (2 ** memoryConfigDecisionBit) + versatBase |> Hex}, 
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
};
