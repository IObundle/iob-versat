#ifndef INCLUDED_CODE_GENERATION
#define INCLUDED_CODE_GENERATION

struct Versat;
struct Accelerator;

struct VersatComputedValues{
   int nConfigs;
   int configBits;

   int nStatics;
   int staticBits;

   int nDelays;
   int delayBits;

   // Configurations = config + static + delays
   int nConfigurations;
   int configurationBits;
   int configurationAddressBits;

   int nStates;
   int stateBits;
   int stateAddressBits;

   int unitsMapped;
   int memoryMappedBytes;
   int maxMemoryMapDWords;

   int nUnitsIO;

   int numberConnections;

   int stateConfigurationAddressBits;
   int memoryAddressBits;
   int memoryMappingAddressBits;
   int memoryConfigDecisionBit;
   int lowerAddressSize;
};

VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel);

#endif // INCLUDED_CODE_GENERATION
