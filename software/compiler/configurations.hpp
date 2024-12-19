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
  Opt<iptr> memMapped; // Used on Code Gen, to create the addresses for the memories.
  Opt<int> memMappedSize; // D
  Opt<int> memMappedBitSize; // D
  Opt<String> memMappedMask; // D
  Opt<String> memDecisionMask; // D
  Opt<int> delayPos; // D 
  Array<int> delay; // 
  int baseDelay; // D
  int delaySize; // D 
  bool isComposite; // D
  bool isMerge;
  bool isStatic; // D
  bool isShared; // D
  int sharedIndex; // D
  FUDeclaration* parent; // D
  String fullName; // D
  int mergePort;
  bool isMergeMultiplexer; // D
  bool belongs; // D
  int special; // D
  int order; // Missing for now
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
  String name;
  Array<int> muxConfigs;
  Array<int> inputDelay;
  Array<int> outputLatencies;
};

struct MergePartition{
  String name;
  Array<InstanceInfo> info;

  FUDeclaration* baseType;
  AcceleratorMapping* mapping; // Maps from base type flattened to merged type baseCircuit
  Set<PortInstance>*  mergeMultiplexers;

  // TODO: All these are useless. We can just store the data in the units themselves.
  Array<int> inputDelays;
  Array<int> outputLatencies;
  Array<int> muxConfigs; // TODO: Remove this. The data should be stored inside the mux Instance Info that it belongs to
};

// TODO: A lot of values are repeated between merge partitions and the like. Not a problem for now, maybe tackle it when things become stable. Or maybe leave it be, could be easier in future if we want to implement something more complex.

struct AccelInfo{
  // An array that abstracts all the common values of any merge type into a single one. Code that does not care about dealing with merge types can access this data
  Array<InstanceInfo> baseInfo; // This shit is already giving errors and causing bugs.

  // The problem is how to go to remove baseInfo without spending too much time.

  // At the same time, I do not want to rewrite the struct generation code, because I eventually want to replace it with something better.

  // The problem is that the "something" better requires better data, which I want to put inside the AccelInfo struct.

  // 
  
  Array<MergePartition> infos;
    
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
  int mergeIndex;

  Array<InstanceInfo>& GetCurrentMerge();
  int MergeSize();

  String GetMergeName();
  
  bool IsValid();
  
  int GetIndex(InstanceInfo* instance);
  
  AccelInfoIterator GetParent();
  InstanceInfo* GetParentUnit();
  InstanceInfo* CurrentUnit();
  Array<InstanceInfo*> GetAllSubUnits(Arena* out);

  // Next and step mimick gdb like commands. Does not update current, instead returning the advanced iterator
  AccelInfoIterator Next() WARN_UNUSED; // Next unit in current level only.
  AccelInfoIterator Step() WARN_UNUSED; // Next unit in the array. Goes down and up the levels as it progresses.
  AccelInfoIterator StepInsideOnly() WARN_UNUSED; // Returns NonValid if non composite unit
};

AccelInfoIterator StartIteration(AccelInfo* info);

struct TypeAndNameOnly{
  String type;
  String name;
};

// TODO: Since we are now following a design where we store all the information that we want in a table, maybe there is a better way of handling this portion. If all the functions that perform different computations worked from the data present in the table, the we could just pass the appropriate table and not have to deal with Partitions.
//       Need to first see which functions depend on this approach, after cleaning up the code with the changes introduced by the table approach, and then take a second look at this.s

//       The problem is that the table gives a global view of the entire configuration space, but partitions is used to give a local view of the global configuration for a given type.
//       We first would need to change from a Array<Array<InstanceInfo>> to Array<SomeStruct> so that we could store data associated to each 

// Maybe the solution would be to move the Partition logic to the AccelInfoIterator? 
// 

// NOTE: I do not want partitions. I do not want the need to have this logic being passed around while being hard to reason about it.

// What I actually want is to get the indexes where a given merge unit is activated.
// If we have 2 merge indexes, and I'm in the merge unit, I should get (0) and (1).
// If we have 4 merge indexes, and I'm in the merge unit, I should get (0,1) and (2,3).
//    If we then go inside the first one, and we arrive at the other merge unit, should get (0) and (1).
//    Basically, if I'm looking at a merge unit, should get the indexes at which the unit is active.

// The problem is how to handle this when going to do the generation part.
// Do we need partitions for calculating the accelerator info of merged entities?
// Our is it possible to generate everything beforehand, including data that allow us to simplify the process?

// The thing about indexes, is that they only exist because of modules.
// It is the fact that modules can instantiate merge units.
// It is instantiation that leads to multiplication of the merges indexes.
//    And instantiation does not change the values that we care about, right? Especially for the structure generation.
// That means that if we merge 4 types, then we only need to care about 4 types. Does not matter that throught modules we might end up with 16 or 32 or any combination of numbers, we only need to look at 4 different types to generate the structures that we need, right?.

// Regardless, need to start dealing with this stuff now, otherwise cannot progress. 

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

