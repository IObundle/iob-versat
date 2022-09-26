#ifndef INCLUDED_VERSAT_HPP
#define INCLUDED_VERSAT_HPP

#include "utils.hpp"

// Forward declarations
struct Versat;
struct Accelerator;
struct FUDeclaration;
struct GraphComputedData;
struct VersatComputedData;

#if 0
struct FUInstance{
	HierarchyName name;

   // Embedded memory
   int* memMapped;
   int* config;
   int* state;
   int* delay;
};
#endif

struct Parameter{
   SizedString name;
   SizedString value;
   Parameter* next;
};

struct FUInstance{
	HierarchyName name;

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

   Parameter* parameterList;

   // Configuration + State variables that versat needs access to
   int done; // Units that are sink or sources of data must implement done to ensure circuit does not end prematurely
   bool isStatic;

   bool namedAccess;
};

#if 0
struct AcceleratorIterator{
   FUDeclaration* type;

   PoolIterator<FUInstance> stack[16];
   int index;
};

AcceleratorIterator IterateByType(Accelerator* accel,FUDeclaration* type);
FUInstance* Next(AcceleratorIterator& iter);
bool HasNext(AcceleratorIterator& iter);
#endif

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times);

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...) __attribute__ ((format (printf, 3, 4)));

// Versat functions
Versat* InitVersat(int base,int numberConfigurations);
void ParseCommandLineOptions(Versat* versat,int argc,const char** argv);
void ParseVersatSpecification(Versat* versat,const char* filepath);
bool SetDebug(Versat* versat,bool flag);

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration declaration);
void OutputVersatSource(Versat* versat,Accelerator* accel,const char* sourceFilepath,const char* constantsFilepath,const char* dataFilepath);
void OutputCircuitSource(Versat* versat,FUDeclaration accelDecl,Accelerator* accel,FILE* file);
void OutputMemoryMap(Versat* versat,Accelerator* accel);
void OutputUnitInfo(FUInstance* instance);

FUDeclaration* GetTypeByName(Versat* versat,SizedString str);

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat);
FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,SizedString entityName);
void RemoveFUInstance(Accelerator* accel,FUInstance* inst);

FUDeclaration* RegisterSubUnit(Versat* versat,SizedString name,Accelerator* accel);

// Can use printf style arguments, but only chars and integers.
// Put arguments right after format string
#define GetInstanceByName(ACCEL,...) GetInstanceByName_(ACCEL,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
FUInstance* GetInstanceByName_(Accelerator* accel,int argc, ...);
FUInstance* GetInstanceByName_(FUInstance* inst,int argc, ...);

void SaveConfiguration(Accelerator* accel,int configuration);
void LoadConfiguration(Accelerator* accel,int configuration);

void AcceleratorRun(Accelerator* accel);

// Helper functions
int GetInputValue(FUInstance* instance,int port);

void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex);
void ConnectUnitsWithDelay(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay);

void VersatUnitWrite(FUInstance* instance,int address, int value);
int VersatUnitRead(FUInstance* instance,int address);

FUDeclaration* MergeAccelerators(Versat* versat,FUDeclaration* accel1,FUDeclaration* accel2);

void Hook(Versat* versat,Accelerator* accel,FUInstance* inst);

#endif // INCLUDED_VERSAT_HPP
