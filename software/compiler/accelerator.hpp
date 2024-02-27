#pragma once

#include <unordered_map>

#include "utils.hpp"
#include "memory.hpp"
#include "declaration.hpp"

struct FUInstance;
struct InstanceNode;
struct OrderedInstance;
struct FUDeclaration;
struct Edge;
struct PortEdge;
struct Versat;
struct FUInstance;

typedef std::unordered_map<FUInstance*,FUInstance*> InstanceMap;
typedef std::unordered_map<PortEdge,PortEdge> PortEdgeMap;
typedef std::unordered_map<Edge*,Edge*> EdgeMap;
typedef Hashmap<InstanceNode*,InstanceNode*> InstanceNodeMap;

struct Accelerator{ // Graph + data storage
  Versat* versat;
  //FUDeclaration* subtype; // Set if subaccelerator (accelerator associated to a FUDeclaration). A "free" accelerator has this set to nullptr

  Edge* edges; // TODO: Should be removed, edge info is all contained inside the instance nodes and desync leads to bugs since some code still uses this.

  OrderedInstance* ordered;
  InstanceNode* allocated;
  InstanceNode* lastAllocated;
  Pool<FUInstance> instances;

  DynamicArena* accelMemory; // TODO: We could remove all this because we can now build all the accelerators in place. (Add an API that functions like a Accelerator builder and at the end we lock everything into an immutable graph).

  std::unordered_map<StaticId,StaticData> staticUnits;

  String name; // For debugging purposes it's useful to give accelerators a name
};

struct VersatComputedData{
  int memoryMaskSize;
  char memoryMaskBuffer[33];
  char* memoryMask;
};

struct ComputedData{
  Array<ExternalMemoryInterface> external; // Just a grouping of all external interfaces.
  Array<VersatComputedData> data;          
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

  int nUnitsIO;
  int numberConnections;
  int externalMemoryInterfaces;

  int stateConfigurationAddressBits;
  int memoryAddressBits;
  int memoryMappingAddressBits;
  int memoryConfigDecisionBit;

  bool signalLoop;
};

// Accelerator

// TODO: The concept of flat instance no longer exists. Remove them and check if any code dependend on the fact that copy flat did not copy static or shared 
Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map);
Accelerator* CopyFlatAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map);
FUInstance* CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName,InstanceNode* previous);
FUInstance* CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName);
InstanceNode* CopyInstance(Accelerator* newAccel,InstanceNode* oldInstance,String newName);
InstanceNode* CreateFlatFUInstance(Accelerator* accel,FUDeclaration* type,String name);

ComputedData CalculateVersatComputedData(Array<InstanceInfo> info,VersatComputedValues val,Arena* out);

bool IsCombinatorial(Accelerator* accel);

Array<FUDeclaration*> ConfigSubTypes(Accelerator* accel,Arena* out,Arena* sub);
Array<FUDeclaration*> MemSubTypes(Accelerator* accel,Arena* out,Arena* sub);

void ReorganizeAccelerator(Accelerator* graph,Arena* temp);
void ReorganizeIterative(Accelerator* accel,Arena* temp);
