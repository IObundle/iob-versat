#pragma once

#include <unordered_map>

#include "memory.hpp"
#include "accelerator.hpp"

struct InstanceInfo{
  int level; // D 
  FUDeclaration* decl; // D
  String name; // D
  String baseName; // D
  Opt<int> configPos; // D  
  int isConfigStatic; // Static must be handle separately, for the top level accelerator. 
  int configSize; // D 
  Opt<int> statePos; // D
  int stateSize; // D
  Opt<iptr> memMapped; // Not doing this, not used a lot and maybe not needed
  Opt<int> memMappedSize; // D
  Opt<int> memMappedBitSize; // D
  Opt<String> memMappedMask; // D
  Opt<String> memDecisionMask; // D
  Opt<int> delayPos; // D 
  Array<int> delay; // TODO // This is the value that the unit delay wire must be set to. How much delay must the unit wait.
  int baseDelay; // D? This is the amount of time it takes for the data to arrive at the unit.
  int delaySize; // D 
  bool isComposite; // D
  bool isStatic; // D
  bool isShared; // D
  int sharedIndex; // D
  FUDeclaration* parent; // D
  String fullName; // D
  bool isMergeMultiplexer; // D
  int mergeMultiplexerId; // D
  bool belongs; // D
  int special; // D
  int order; // Not needed if we sort the units beforehand.
  NodeType connectionType; // D
  int id; // D
  int mergeIndexStart; // D
  FUInstance* inst; // D
};

struct AcceleratorInfo{
  Array<InstanceInfo> info;
  int memSize;
  Opt<String> name;
  Array<int> mergeMux;
};

struct InstanceConfigurationOffsets{
  Hashmap<StaticId,int>* staticInfo; 
  FUDeclaration* parent;
  String topName;
  String baseName;
  int configOffset;
  int stateOffset;
  int delayOffset;
  int delay; // Actual delay value, not the delay offset.
  int memOffset;
  int level;
  int order;
  int* staticConfig; // This starts at 0x40000000 and at the end of the function we normalized it since we can only figure out the static position at the very end.
  bool belongs;
};

struct TestResult{
  Array<InstanceInfo> info;
  InstanceConfigurationOffsets subOffsets;
  Array<PortInstance> multiplexersPorts;
  String name;
  Array<int> muxConfigs;
  Array<int> inputDelay;
  Array<int> outputLatencies;
  int mergeIndex;
};

struct AccelInfo{
  // An array that abstracts all the common values of any merge type into a single one. Code that does not care about dealing with merge types can access this data
  Array<InstanceInfo> baseInfo;

  // Each array contains one value for each merge type.
  Array<Array<InstanceInfo>> infos; // Should join names with infos
  Array<String> names; // Merge type names
  Array<Array<int>> inputDelays;
  Array<Array<int>> outputDelays;
  Array<Array<int>> muxConfigs; // The values that the multiplexer instances must be configured to in order to enable the datapath associated to the type 
  
  int memMappedBitsize;
  int howManyMergedUnits;
  
  int inputs;
  int outputs;

  int configs;
  int states;
  int delays;
  int ios;
  int statics;
  int staticBits;
  int sharedUnits;
  int externalMemoryInterfaces;
  int externalMemoryByteSize;
  int numberUnits;
  int numberConnections;
  
  int memoryMappedBits;
  bool isMemoryMapped;
  bool signalLoop;
};

// NOTE: The member 'level' of InstanceInfo needs to be valid in order for this iterator to work. 
//       Do not know how to handle merged. Should we iterate Array<InstanceInfo> and let outside code work, or do we take the accelInfo and then allow the iterator to switch between different merges and stuff?      
struct AccelInfoIterator{
  AccelInfo* info;
  int index;

  bool IsValid();

  int GetIndex(InstanceInfo* instance);
  InstanceInfo* GetParentUnit();
  InstanceInfo* CurrentUnit();
  Array<InstanceInfo*> GetAllSubUnits(Arena* out);

  // Next and step mimick gdb like commands
  AccelInfoIterator Next(); // Next unit in current level only.
  AccelInfoIterator Step(); // Next unit in the array. Goes down and up the levels as it progresses.
  AccelInfoIterator StepInsideOnly(); // Returns NonValid if non composite unit
};

AccelInfoIterator StartIteration(AccelInfo* info);

struct TypeAndNameOnly{
  String type;
  String name;
};

// TODO: Since we are now following a design where we store all the information that we want in a table, maybe there is a better way of handling this portion. If all the functions that perform different computations worked from the data present in the table, the we could just pass the appropriate table and not have to deal with Partitions.
//       Need to first see which functions depend on this approach, after cleaning up the code with the changes introduced by the table approach, and then take a second look at this.s
struct Partition{
  int value;
  int max;
  int mergeIndexStart;
};

// Kinda temp having to export this function, but maybe this will become permanent. (With a new name)
Array<InstanceInfo> CalculateInstanceInfoTest(Accelerator* accel,Arena* out,Arena* temp);

int GetConfigIndexFromInstance(Accelerator* accel,FUInstance* inst);

AccelInfo CalculateAcceleratorInfo(Accelerator* accel,bool recursive,Arena* out,Arena* temp);
AccelInfo CalculateAcceleratorInfoNoDelay(Accelerator* accel,bool recursive,Arena* out,Arena* temp);

CalculatedOffsets CalculateConfigOffsetsIgnoringStatics(Accelerator* accel,Arena* out);
CalculatedOffsets CalculateConfigurationOffset(Accelerator* accel,MemType type,Arena* out);

Array<Partition> GenerateInitialPartitions(Accelerator* accel,Arena* out);

int GetConfigurationSize(FUDeclaration* decl,MemType type);

// Array info related
Array<Wire> ExtractAllConfigs(Array<InstanceInfo> info,Arena* out,Arena* temp);
Array<String> ExtractStates(Array<InstanceInfo> info,Arena* out);
Array<Pair<String,int>> ExtractMem(Array<InstanceInfo> info,Arena* out);

void ShareInstanceConfig(FUInstance* instance, int shareBlockIndex);
void SetStatic(Accelerator* accel,FUInstance* instance);

String ReprStaticConfig(StaticId id,Wire* wire,Arena* out);

struct OrderedConfigurations{
   Array<Wire> configs;
   Array<Wire> statics;
   Array<Wire> delays;
};

Array<InstanceInfo> ExtractFromInstanceInfoSameLevel(Array<InstanceInfo> instanceInfo,int level,Arena* out);

void CheckSanity(Array<InstanceInfo> instanceInfo,Arena* temp);

void PrintConfigurations(FUDeclaration* type,Arena* temp);

