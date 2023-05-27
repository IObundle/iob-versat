#ifndef INCLUDED_VERSAT_DECLARATION
#define INCLUDED_VERSAT_DECLARATION

#include <cstdio>

#include "configurations.hpp"

struct ComplexFUInstance;

static const int VCD_MAPPING_SIZE = 5;
class VCDMapping{
public:
   char currentMapping[VCD_MAPPING_SIZE+1];
   int increments;

public:

   VCDMapping(){Reset();};

   void Increment(){
      for(int i = VCD_MAPPING_SIZE - 1; i >= 0; i--){
         increments += 1;
         currentMapping[i] += 1;
         if(currentMapping[i] == 'z' + 1){
            currentMapping[i] = 'a';
         } else {
            return;
         }
      }
      Assert(false && "Increase mapping space");
   }

   void Reset(){
      currentMapping[VCD_MAPPING_SIZE] = '\0';
      for(int i = 0; i < VCD_MAPPING_SIZE; i++){
         currentMapping[i] = 'a';
      }
      increments = 0;
   }

   char* Get(){
      return currentMapping;
   }
};

typedef int* (*FUFunction)(ComplexFUInstance* inst);
typedef int* (*FUUpdateFunction)(ComplexFUInstance* inst,Array<int> inputs);
typedef int (*MemoryAccessFunction)(ComplexFUInstance* inst, int address, int value,int write);
typedef void (*VCDFunction)(ComplexFUInstance*,FILE*,VCDMapping&,Array<int>,bool firstTime,bool printDefinitions);

struct Wire{
   String name;
   int bitsize;
};

// TODO: Some structures appear to hold more data that necessary.
//       The current way data is modelled is weird
//       Until I get more different memory types or
//       until we allow certain wires to not be used,
//       It's hard to figure out how to proceed
//       Let this be for now
enum ExternalMemoryType{TWO_P = 0,DP};

struct ExternalMemoryInterface{
   int interface;
   int bitsize;
   int datasize;
   ExternalMemoryType type;
};

struct ExternalMemoryID{
   int interface;
   ExternalMemoryType type;
};

struct ExternalPortInfo{
   int addrSize;
   int dataInSize;
   int dataOutSize;
   bool enable;
   bool write;
};

// Contain info parsed directly by verilog.
// This probably should be a union of all the memory types
// The code in the verilog parser almost demands it
struct ExternalMemoryInfo{
   int numberPorts;

   // Maximum of 2 ports
   ExternalPortInfo ports[2];
};

enum DelayType {
   DELAY_TYPE_BASE               = 0x0,
   DELAY_TYPE_SINK_DELAY         = 0x1,
   DELAY_TYPE_SOURCE_DELAY       = 0x2,
   DELAY_TYPE_COMPUTE_DELAY      = 0x4
};
#define CHECK_DELAY(inst,T) ((inst->declaration->delayType & T) == T)

inline DelayType operator|(DelayType a, DelayType b)
{return static_cast<DelayType>(static_cast<int>(a) | static_cast<int>(b));}

// A declaration is constant after being registered
struct FUDeclaration{
   String name;

   Array<int> inputDelays;
   Array<int> outputLatencies;

   Array<Wire> configs;
   Array<Wire> states;

   CalculatedOffsets configOffsets;
   CalculatedOffsets stateOffsets;
   CalculatedOffsets delayOffsets;
   CalculatedOffsets outputOffsets;
   CalculatedOffsets extraDataOffsets;

   Array<iptr> defaultConfig;
   Array<iptr> defaultStatic;

   int nIOs;
   int memoryMapBits;
   int nStaticConfigs;

   Array<ExternalMemoryInterface> externalMemory;

   // Stores different accelerators depending on properties we want
   Accelerator* baseCircuit;
   Accelerator* fixedDelayCircuit;

   // Merged accelerator
   Array<FUDeclaration*> mergedType;

   FUFunction initializeFunction;
   FUFunction startFunction;
   FUUpdateFunction updateFunction;
   FUFunction destroyFunction;
   VCDFunction printVCD;
   MemoryAccessFunction memAccessFunction;

   const char* operation;

   // TODO: this is probably not needed, only used for verilog generation (which could be calculated inside the code generation functions)
   //       the info is all contained inside the units themselves.
   Hashmap<StaticId,StaticData>* staticUnits;

   enum {SINGLE = 0x0,COMPOSITE = 0x1,SPECIAL = 0x2,MERGED = 0x3,ITERATIVE = 0x4} type;
   DelayType delayType;

   bool isOperation;
   bool implementsDone;
   bool isMemoryMapped;
};

bool IsCombinatorial(FUDeclaration* decl);


#endif // INCLUDED_VERSAT_DECLARATION
