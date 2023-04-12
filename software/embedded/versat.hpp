#ifndef INCLUDED_VERSAT
#define INCLUDED_VERSAT

#include <stdint.h>
#include <stddef.h>

#include "utilsCore.hpp"

struct FUInstance{
   const char* name;
   volatile int* memMapped;
   volatile int* config;
   volatile int* state;
   volatile int* delay;

   int numberChilds;
};

struct Versat{
};

struct FUDeclaration{
};

struct SimpleAccelerator;

struct Accelerator{
   Versat* versat;
   bool locked;
};

struct SpecificMerge{
   String instA;
   String instB;
};

enum VersatDebugFlags{
   OUTPUT_GRAPH_DOT,
   GRAPH_DOT_FORMAT,
   OUTPUT_ACCELERATORS_CODE,
   OUTPUT_VERSAT_CODE,
   OUTPUT_VCD,
   USE_FIXED_BUFFERS
};

typedef uint GraphDotFormat;
enum GraphDotFormat_{
   GRAPH_DOT_FORMAT_NAME = 0x1,
   GRAPH_DOT_FORMAT_TYPE = 0x2,
   GRAPH_DOT_FORMAT_ID = 0x4,
   GRAPH_DOT_FORMAT_DELAY = 0x8,
   GRAPH_DOT_FORMAT_EXPLICIT = 0x10, // indicates which field is which when outputting name
   GRAPH_DOT_FORMAT_PORT = 0x20, // Outputs port information for edges and port instance
   GRAPH_DOT_FORMAT_LATENCY = 0x40 // Outputs latency information for edges and port instances which know their latency information
};

enum MergingStrategy{
   SIMPLE_COMBINATION,
   CONSOLIDATION_GRAPH,
   PIECEWISE_CONSOLIDATION_GRAPH,
   FIRST_FIT,
   ORDERED_FIT
};

Versat* InitVersat(int base,int numberConfigurations);

//Arena InitArena(size_t size);

// Accelerator functions
void AcceleratorRun(Accelerator* accel);
void VersatUnitWrite(FUInstance* instance,int address, int value);
int32_t VersatUnitRead(FUInstance* instance,int address);

#define GetInstanceByName(ACCEL,...) GetInstanceByName_(ACCEL,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
FUInstance* GetInstanceByName_(Accelerator* accel,int argc, ...);

#define GetSubInstanceByName(TOP_LEVEL,INST,...) GetSubInstanceByName_(TOP_LEVEL,INST,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
FUInstance* GetSubInstanceByName_(Accelerator* topLevel,FUInstance* inst,int argc, ...);

void CalculateDelay(Versat* versat,Accelerator* accel); // In versat space, simple extract delays from configuration data
FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String entityName);

void Hook(Versat* versat,Accelerator* accel,FUInstance* inst);
// Functions that perform no useful work are simple pre processed out
inline uint SetDebug(Versat* versat,VersatDebugFlags flags,uint flag){return 0;}
inline void ClearConfigurations(Accelerator* accel){}
inline void ActivateMergedAccelerator(Versat* versat,Accelerator* accel,FUDeclaration* type){} // Set the accelerator configuration to execute the merged graph represented by type
#define CreateAccelerator(...) ((Accelerator*)0)
#define RegisterFU(...) ((FUDeclaration*)0)
#define DisplayAcceleratorMemory(...) ((void)0)
inline void OutputVersatSource(Versat* versat,Accelerator* accel,const char* directoryPath){}
inline void OutputVersatSource(Versat* versat,SimpleAccelerator* accel,const char* directoryPath){}
#define ConnectUnits(...) ((void)0)
#define OutputGraphDotFile(...) ((void)0)
#define RemoveFUInstance(...) ((void)0)
#define SetDelayRecursive(...) ((void)0)
#define Flatten(...) ((Accelerator*)0)
#define GetTypeByName(...) ((FUDeclaration*)0)
#define ParseCommandLineOptions(...) ((void)0)
#define OutputMemoryMap(...) ((void)0)
#define OutputUnitInfo(...) ((void)0)
#define RegisterTypes(...) ((void)0)
#define ParseVersatSpecification(...) ((void)0)
#define MergeAccelerators(...) ((FUDeclaration*)0)
inline void Free(Versat* versat){}
#define DisplayUnitConfiguration(...) ((void)0)
#define CheckInputAndOutputNumber(...) ((void)0)
#define MergeThree(...) ((FUDeclaration*)0)
#define Merge(...) ((FUDeclaration*)0)
inline void TestVersatSide(Versat* versat){}

#endif // INCLUDED_VERSAT
