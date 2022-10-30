#ifndef INCLUDED_VERSAT_HPP
#define INCLUDED_VERSAT_HPP

#include "utils.hpp"

// Forward declarations
struct Versat;
struct Accelerator;
struct FUDeclaration;
struct GraphComputedData;
struct VersatComputedData;

struct FUInstance{
	//HierarchyName name;
   SizedString name;

   // Embedded memory
   int* memMapped;
   int* config;
   int* state;
   int* delay;

   // PC only
   int baseDelay;

   Accelerator* accel;
	FUDeclaration* declaration;
	int id;
   Accelerator* compositeAccel;

	int* outputs;
	int* storedOutputs;
   char* extraData;

   SizedString parameters;

   // Configuration + State variables that versat needs access to
   int done; // Units that are sink or sources of data must implement done to ensure circuit does not end prematurely
   bool isStatic;

   bool namedAccess;
};

enum VersatDebugFlags{
   OUTPUT_GRAPH_DOT,
   OUTPUT_ACCELERATORS_CODE,
   OUTPUT_VERSAT_CODE,
   OUTPUT_VCD,
   USE_FIXED_BUFFERS
};

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times);

void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...) __attribute__ ((format (printf, 4, 5)));

// Versat functions
Versat* InitVersat(int base,int numberConfigurations);
void Free(Versat* versat);
void ParseCommandLineOptions(Versat* versat,int argc,const char** argv);
void ParseVersatSpecification(Versat* versat,const char* filepath);

bool SetDebug(Versat* versat,VersatDebugFlags flags, bool flag);

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration declaration);
void OutputVersatSource(Versat* versat,Accelerator* accel,const char* sourceFilepath,const char* constantsFilepath,const char* dataFilepath);
void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file);
void OutputMemoryMap(Versat* versat,Accelerator* accel);
void OutputUnitInfo(FUInstance* instance);

FUDeclaration* GetTypeByName(Versat* versat,SizedString str);

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat);
FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,SizedString entityName,bool flat = false,bool isStatic = false);
void RemoveFUInstance(Accelerator* accel,FUInstance* inst);

FUDeclaration* RegisterSubUnit(Versat* versat,SizedString name,Accelerator* accel);

// Can use printf style arguments, but only chars and integers.
// Put arguments right after format string
#define GetInstanceByName(ACCEL,...) GetInstanceByName_(ACCEL,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
FUInstance* GetInstanceByName_(Accelerator* accel,int argc, ...);
FUInstance* GetInstanceByName_(FUInstance* inst,int argc, ...);

void ClearConfigurations(Accelerator* accel);
void SaveConfiguration(Accelerator* accel,int configuration);
void LoadConfiguration(Accelerator* accel,int configuration);

void AcceleratorRun(Accelerator* accel);
void ActivateMergedAccelerator(Versat* versat,Accelerator* accel,FUDeclaration* type);

//void PopulateAccelerator(Accelerator* accel);
void CheckMemory(Accelerator* topLevel,Accelerator* accel);
void DisplayAcceleratorMemory(Accelerator* topLevel);

void SetDefaultConfiguration(FUInstance* inst,int* config,int size);
void ShareInstanceConfig(FUInstance* inst, int shareBlockIndex);

// Helper functions
int GetInputValue(FUInstance* instance,int port);

void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex);
void ConnectUnitsWithDelay(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay);

void VersatUnitWrite(FUInstance* instance,int address, int value);
int VersatUnitRead(FUInstance* instance,int address);

FUDeclaration* MergeAccelerators(Versat* versat,FUDeclaration* accel1,FUDeclaration* accel2,SizedString name);

void Hook(Versat* versat,Accelerator* accel,FUInstance* inst);

#endif // INCLUDED_VERSAT_HPP
