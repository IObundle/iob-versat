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
static volatile int* configBase = (volatile int*) @{versatBase + 4 |> Hex};
static volatile int* stateBase = (volatile int*) @{versatBase + 4 |> Hex};
static volatile int* memMappedBase = (volatile int*) @{versatBase + memoryMappedBase * 4 |> Hex};

#{set delaySeen 0}
FUInstance instancesBuffer[] = {
   #{join "," for inst instances}
   {
      .name = "@{inst.name.str}",
      #{if inst.declaration.isMemoryMapped} 
         .memMapped = (int*) @{versatBase + memoryMappedBase * 4 + inst.versatData.memoryAddressOffset * 4 |> Hex}, 
      #{else} 
         .memMapped = (int*) 0x0, 
      #{end}
      
      #{if inst.config >= static and inst.config < staticEnd}
         .config = (int*) @{inst.config - static + (nConfigs * 4) + versatBase |> Hex},
      #{else}
         #{if inst.config} 
            .config = (int*) @{inst.config - config + 4 + versatBase |> Hex}, 
         #{else} 
            .config = (int*) 0x0, 
         #{end}
      #{end}
      
      #{if inst.state} 
         .state = (int*) @{inst.state - state + 4 + versatBase |> Hex},
      #{else} 
         .state = (int*) 0x0,
      #{end}

      #{if inst.declaration.nDelays}
         .delay = (int*) &delayBase[@{delaySeen}],
      #{set delaySeen delaySeen + inst.declaration.nDelays}
      #{else}
         .delay = (int*) 0x0,
      #{end}

      .numberChilds = @{inst.compositeAccel |> CountNonOperationChilds}
   }
   #{end}
};
