#ifndef INCLUDED_VERSAT_PRIVATE_HPP
#define INCLUDED_VERSAT_PRIVATE_HPP

#include <vector>
#include <unordered_map>

#include "memory.hpp"
#include "logger.hpp"

#include "versat.hpp"
#include "accelerator.hpp"
#include "configurations.hpp"
#include "merge.hpp"
#include "graph.hpp"

struct InstanceNode;
struct ComplexFUInstance;
struct Versat;

struct DAGOrder{
   ComplexFUInstance** sinks;
   int numberSinks;
   ComplexFUInstance** sources;
   int numberSources;
   ComplexFUInstance** computeUnits;
   int numberComputeUnits;
   ComplexFUInstance** instances;
};

struct VersatComputedData{
   int memoryMaskSize;
   int memoryAddressOffset;
   char memoryMaskBuffer[33];
   char* memoryMask;
};

struct ComputedData{
   Array<ExternalMemoryInterface> external;
   Array<VersatComputedData> data;
};

struct Parameter{
   String name;
   String value;
   Parameter* next;
};

struct DAGOrderNodes{
   InstanceNode** sinks;
   int numberSinks;
   InstanceNode** sources;
   int numberSources;
   InstanceNode** computeUnits;
   int numberComputeUnits;
   InstanceNode** instances;
   int* order;
   int size;
   int maxOrder;
};

struct ComplexFUInstance : public FUInstance{
   // PC only
   int baseDelay;

   // Configuration + State variables that versat needs access to
   int done; // Units that are sink or sources of data must implement done to ensure circuit does not end prematurely
   bool isStatic;

   bool namedAccess;

   // Various uses
   Accelerator* iterative;

   UnitDebugData* debugData;

   union{
      int literal;
      int bufferAmount;
      int portIndex;
   };
   //GraphComputedData* graphData;
   //VersatComputedData* versatData;
   int sharedIndex;
   char tag;
   bool savedConfiguration; // For subunits registered, indicate when we save configuration before hand
   bool savedMemory; // Same for memory
   bool sharedEnable;
   bool initialized;
};

struct DebugState{
   uint dotFormat;
   bool outputGraphs;
   bool outputAccelerator;
   bool outputVersat;
   bool outputVCD;
   bool useFixedBuffers;
};

struct Versat{
	Pool<FUDeclaration> declarations;
	Pool<Accelerator> accelerators;

	Arena permanent;
	Arena temp;

	int base;
	int numberConfigurations;

   DebugState debug;
};

struct UnitValues{
   int inputs;
   int outputs;

   int configs;
   int states;
   int delays;
   int ios;
   int totalOutputs;
   int extraData;
   int statics;
   int sharedUnits;
   int externalMemoryInterfaces;
   int externalMemorySize;
   int numberUnits;

   int memoryMappedBits;
   bool isMemoryMapped;
};

struct VersatComputedValues{
   int nConfigs;
   int configBits;

   int versatConfigs;
   int versatStates;

   int nStatics;
   int staticBits;
   int staticBitsStart;

   int nDelays;
   int delayBits;
   int delayBitsStart;

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

   int externalMemoryInterfaces;

   int stateConfigurationAddressBits;
   int memoryAddressBits;
   int memoryMappingAddressBits;
   int memoryConfigDecisionBit;
   int lowerAddressSize;
};

struct HierarchicalName{
   String name;
   HierarchicalName* next;
};

struct SharingInfo{
   int* ptr;
   bool init;
};
// Simple operations should also be stored here. They are versat agnostic as well
namespace BasicDeclaration{
	extern FUDeclaration* buffer;
   extern FUDeclaration* fixedBuffer;
   extern FUDeclaration* input;
   extern FUDeclaration* output;
   extern FUDeclaration* multiplexer;
   extern FUDeclaration* combMultiplexer;
   extern FUDeclaration* timedMultiplexer;
   extern FUDeclaration* stridedMerge;
   extern FUDeclaration* pipelineRegister;
   extern FUDeclaration* data;
}

struct CompiledTemplate;
namespace BasicTemplates{
   extern CompiledTemplate* acceleratorTemplate;
   extern CompiledTemplate* topAcceleratorTemplate;
   extern CompiledTemplate* dataTemplate;
   extern CompiledTemplate* unitVerilogData;
   extern CompiledTemplate* acceleratorHeaderTemplate;
   extern CompiledTemplate* externalPortmapTemplate;
   extern CompiledTemplate* externalPortTemplate;
   extern CompiledTemplate* externalInstTemplate;
   extern CompiledTemplate* iterativeTemplate;
}

struct GraphMapping;

// Temp
bool EqualPortMapping(PortInstance p1,PortInstance p2);

// General info
UnitValues CalculateIndividualUnitValues(ComplexFUInstance* inst); // Values for individual unit, not taking into account sub units. For a composite, this pretty much returns empty except for total outputs, as the unit itself must allocate output memory
UnitValues CalculateAcceleratorUnitValues(Versat* versat,ComplexFUInstance* inst); // Values taking into account sub units
UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel);
VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel);
int CalculateTotalOutputs(Accelerator* accel);
int CalculateTotalOutputs(ComplexFUInstance* inst);
bool IsConfigStatic(Accelerator* topLevel,ComplexFUInstance* inst);
bool IsUnitCombinatorial(ComplexFUInstance* inst);

int CalculateNumberOfUnits(InstanceNode* node);

// Delay
int CalculateLatency(ComplexFUInstance* inst);
Hashmap<EdgeNode,int>* CalculateDelay(Versat* versat,Accelerator* accel,Arena* out);
void SetDelayRecursive(Accelerator* accel,Arena* arena);

// Graph fixes
void FixMultipleInputs(Versat* versat,Accelerator* accel);
void FixMultipleInputs(Versat* versat,Accelerator* accel,Hashmap<ComplexFUInstance*,int>* instanceToInput);
void FixDelays(Versat* versat,Accelerator* accel,Hashmap<EdgeNode,int>* edgeDelays);

// Accelerator merging
DAGOrderNodes CalculateDAGOrder(InstanceNode* instances,Arena* arena);

// Debug
void AssertGraphValid(InstanceNode* nodes,Arena* arena);
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...) __attribute__ ((format (printf, 4, 5)));
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,ComplexFUInstance* highlighInstance,const char* filenameFormat,...) __attribute__ ((format (printf, 5, 6)));
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,Set<ComplexFUInstance*>* highlight,const char* filenameFormat,...) __attribute__ ((format (printf, 5, 6)));
Array<String> GetFullNames(Accelerator* accel,Arena* out);

void TemplateSetDefaults(Versat*);

// Misc
bool CheckValidName(String name); // Check if name can be used as identifier in verilog
bool IsTypeHierarchical(FUDeclaration* decl);
bool IsTypeSimulatable(FUDeclaration* decl);
int CountNonSpecialChilds(InstanceNode* nodes);
void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file);
InstanceNode* GetInputNode(InstanceNode* nodes,int inputIndex);
int GetInputValue(InstanceNode* nodes,int index);
ComplexFUInstance* GetInputInstance(InstanceNode* nodes,int inputIndex);
InstanceNode* GetOutputNode(InstanceNode* nodes);
ComplexFUInstance* GetOutputInstance(InstanceNode* nodes);
PortNode GetInputValueInstance(ComplexFUInstance* inst,int index);

ComputedData CalculateVersatComputedData(InstanceNode* instances,VersatComputedValues val,Arena* out);

#include "typeSpecifics.inl"


#endif // INCLUDED_VERSAT_PRIVATE_HPP
