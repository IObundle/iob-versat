#ifndef INCLUDED_VERSAT_HPP
#define INCLUDED_VERSAT_HPP

#include "utils.hpp"

// Forward declarations
struct Versat;
struct Accelerator;
struct FUDeclaration;
struct IterativeUnitDeclaration; // TODO: Cannot leak this struct to public

struct FUInstance{
   String name;

   // Embedded memory
   int* memMapped;
   int* config;
   int* state;
   int* delay;
   int* externalMemory;

   // PC only
   int baseDelay;

   Accelerator* accel;
	FUDeclaration* declaration;
	int id;

	int* outputs;
	int* storedOutputs;
   unsigned char* extraData;

   String parameters;

   // Configuration + State variables that versat needs access to
   int done; // Units that are sink or sources of data must implement done to ensure circuit does not end prematurely
   bool isStatic;

   bool namedAccess;
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

enum MemType{
   CONFIG,
   STATE,
   DELAY,
   STATIC,
   EXTRA,
   OUTPUT,
   STORED_OUTPUT
};

// Temp
Accelerator* RecursiveFlatten(Versat* versat,Accelerator* topLevel);

// Versat functions
Versat* InitVersat(int base,int numberConfigurations);
void Free(Versat* versat); // Usually not needed, as memory is freed on program termination and Versat is supposed to be "active" from start to finish, but usuful for debugging memory problems
void ParseCommandLineOptions(Versat* versat,int argc,const char** argv);
uint SetDebug(Versat* versat,VersatDebugFlags flags,uint flag);
void ParseVersatSpecification(Versat* versat,String content);
void ParseVersatSpecification(Versat* versat,const char* filepath);

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat);
FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String entityName);
void AcceleratorRun(Accelerator* accel);
void AcceleratorRunDebug(Accelerator* accel);
void RemoveFUInstance(Accelerator* accel,FUInstance* inst);
void OutputVersatSource(Versat* versat,Accelerator* accel,const char* sourceFilepath,const char* constantsFilepath,const char* dataFilepath);
Accelerator* Flatten(Versat* versat,Accelerator* accel,int times); // This should work on the passed accelerator. We should not create a new accelerator

// Access units and sub units inside an accelerator. Can use printf style arguments, but only chars and integers are currently supported. Format is <format1>,<args1 if any>,<format2>,...
#define GetInstanceByName(ACCEL,...) GetInstanceByName_(ACCEL,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
FUInstance* GetInstanceByName_(Accelerator* accel,int argc, ...);
FUInstance* GetInstanceByName_(FUDeclaration* decl,int argc, ...);

#define GetSubInstanceByName(TOP_LEVEL,INST,...) GetSubInstanceByName_(TOP_LEVEL,INST,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
FUInstance* GetSubInstanceByName_(Accelerator* topLevel,FUInstance* inst,int argc, ...);

// Unit connection
void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);

// Read/Write to unit
void VersatUnitWrite(FUInstance* instance,int address, int value);
int VersatUnitRead(FUInstance* instance,int address);

// Unit default configuration and configuration sharing
void SetDefaultConfiguration(FUInstance* inst);
void ShareInstanceConfig(FUInstance* inst, int shareBlockIndex);
void SetStatic(Accelerator* accel,FUInstance* inst);

// Declaration functions
FUDeclaration* RegisterFU(Versat* versat,FUDeclaration declaration);
FUDeclaration* GetTypeByName(Versat* versat,String str);
FUDeclaration* RegisterIterativeUnit(Versat* versat,IterativeUnitDeclaration* decl); // TODO: Cannot let the IterativeUnitDeclaration leak to the public interface.
FUDeclaration* RegisterSubUnit(Versat* versat,String name,Accelerator* accel);
FUDeclaration* MergeAccelerators(Versat* versat,FUDeclaration* accel1,FUDeclaration* accel2,String name,int flatteningOrder = 99,
                                 MergingStrategy strategy = MergingStrategy::CONSOLIDATION_GRAPH,SpecificMerge* specifics = nullptr,int nSpecifics = 0);

// Configuration loading and storing
void ClearConfigurations(Accelerator* accel);
void SaveConfiguration(Accelerator* accel,int configuration);
void LoadConfiguration(Accelerator* accel,int configuration);
void ActivateMergedAccelerator(Versat* versat,Accelerator* accel,FUDeclaration* type); // Set the accelerator configuration to execute the merged graph represented by type

// Debug + output general accelerator info
void OutputMemoryMap(Versat* versat,Accelerator* accel);
void OutputUnitInfo(FUInstance* instance);
void DisplayAcceleratorMemory(Accelerator* topLevel);
void DisplayUnitConfiguration(Accelerator* topLevel);
void EnterDebugTerminal(Versat* versat);
void CheckMemory(Accelerator* topLevel);
void CheckMemory(Accelerator* topLevel,MemType type);

#ifdef x86
#define DebugAccelerator(...) ((void)0)
#else
void DebugAccelerator(Accelerator* accel,Arena* temp);
#endif

// Debug units, only works for pc-emul (no declaration info in embedded)
bool CheckInputAndOutputNumber(FUDeclaration* type,int inputs,int outputs);
void PrintAcceleratorInstances(Accelerator* accel);

// Helper functions, useful to implement custom units
int GetInputValue(FUInstance* instance,int port);
int GetNumberOfInputs(FUInstance* inst);
int GetNumberOfOutputs(FUInstance* inst);
FUInstance* CreateOrGetInput(Accelerator* accel,String name,int portNumber);

// Helper functions to create sub accelerators
int GetNumberOfInputs(Accelerator* accel);
int GetNumberOfOutputs(Accelerator* accel);
void SetInputValue(Accelerator* accel,int portNumber,int number);
int GetOutputValue(Accelerator* accel,int portNumber);
int GetInputPortNumber(FUInstance* inputInstance);

// General hook function for debugging purposes
int CalculateMemoryUsage(Versat* versat); // Not accurate, but returns the biggest amount of memory usage.
void Hook(Versat* versat,FUDeclaration* decl,Accelerator* accel,FUInstance* inst);

void TestVersatSide(Versat* versat); // Calls tests from versat side

FUDeclaration* MergeThree(Versat* versat,FUDeclaration* typeA,FUDeclaration* typeB,FUDeclaration* typeC);
FUDeclaration* Merge(Versat* versat,Array<FUDeclaration*> types,String name,MergingStrategy strat = MergingStrategy::CONSOLIDATION_GRAPH);

#endif // INCLUDED_VERSAT_HPP
