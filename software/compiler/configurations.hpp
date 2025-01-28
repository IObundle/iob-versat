#pragma once

#include <unordered_map>

#include "memory.hpp"
#include "accelerator.hpp"

struct SimplePortInstance{
  int inst;
  int port;
};

struct SimplePortConnection{
  int outInst;
  int outPort;
  int inPort;
};

// We currently just stuff everything into this struct, so it's easier to visualize all the info that we need for
// the current accelerator.
// Some of this data is duplicated/unnecessary, but for now we just carry on since this simplifies debugging a lot, being able to see all the info for a given accelerator directly.
struct InstanceInfo{
  int level;
  FUDeclaration* decl;
  String name;
  String baseName;
  Opt<int> configPos;
  int isConfigStatic; // Static must be handle separately, for the top level accelerator. 
  int configSize;
  Opt<int> statePos;
  int stateSize;
  
  // Some of these could be removed and be computed from the others
  Opt<iptr> memMapped; // Used on Code Gen, to create the addresses for the memories.
  Opt<int> memMappedSize;
  Opt<int> memMappedBitSize;
  Opt<String> memDecisionMask; // This is local to the accelerator
  Opt<int> delayPos;
  Array<int> delay;
  int baseDelay;
  int delaySize;
  bool isComposite;
  bool isMerge;
  bool isStatic;
  bool isShared;
  int sharedIndex;
  FUDeclaration* parent;
  String fullName;
  int mergePort;
  bool isMergeMultiplexer;
  bool belongs;
  int special;
  int localOrder;
  NodeType connectionType;
  int id;
  FUInstance* inst;
  Array<int> inputDelays;
  Array<int> outputLatencies;
  Array<int> portDelay;

  Array<SimplePortConnection> inputs; 
};

struct MergePartition{
  String name; // For now this appears to not affect anything.
  Array<InstanceInfo> info;

  // TODO: Composite units currently break the meaning of baseType.
  //       Since a composite unit with 2 merged instances would have 2 base types.
  FUDeclaration* baseType;
  AcceleratorMapping* baseTypeFlattenToMergedBaseCircuit;
  Set<PortInstance>*  mergeMultiplexers;

  // TODO: All these are useless. We can just store the data in the units themselves.
  Array<int> inputDelays;
  Array<int> outputLatencies;
  Array<int> muxConfigs; // TODO: Remove this. The data should be stored inside the mux Instance Info that it belongs to
};

// TODO: A lot of values are repeated between merge partitions and the like. Not a problem for now, maybe tackle it when things become stable. Or maybe leave it be, could be easier in future if we want to implement something more complex.
struct AccelInfo{
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
  String accelName; // Usually for debug purposes.
  AccelInfo* info;
  int index;
  int mergeIndex;

  Array<InstanceInfo>& GetCurrentMerge();
  int MergeSize();

  String GetMergeName();
  
  bool IsValid();
  
  int GetIndex();
  int GetIndex(InstanceInfo* instance);
  
  AccelInfoIterator GetParent();
  InstanceInfo* GetParentUnit();
  InstanceInfo* CurrentUnit();
  InstanceInfo* GetUnit(int index);
  Array<InstanceInfo*> GetAllSubUnits(Arena* out);

  // Next and step mimick gdb like commands. Does not update current, instead returning the advanced iterator
  WARN_UNUSED AccelInfoIterator Next(); // Next unit in current level only.
  WARN_UNUSED AccelInfoIterator Step(); // Next unit in the array. Goes down and up the levels as it progresses.
  WARN_UNUSED AccelInfoIterator StepInsideOnly(); // Returns NonValid if non composite unit

  int CurrentLevelSize();
};

struct Partition{
  int value;
  int max;
  int mergeIndexStart;
  FUDeclaration* decl;
};

AccelInfoIterator StartIteration(AccelInfo* info);

// TODO: We kinda wanted to remove partitions and replace them with the AccelInfoIter approach, but the concept appears multiple times, so we probably do need to be able to represent partitions outside of the iter approach.
Array<AccelInfoIterator> GetCurrentPartitionsAsIterators(AccelInfoIterator iter,Arena* out);
AccelInfoIterator GetCurrentPartitionTypeAsIterator(AccelInfoIterator iter,Arena* out);
int GetPartitionIndex(AccelInfoIterator iter);

Array<InstanceInfo*> GetAllSameLevelUnits(AccelInfo* info,int level,int mergeIndex,Arena* out);

// TODO: mergeIndex seems to be the wrong approach. Check the correct approach when trying to simplify merge.
Array<InstanceInfo> GenerateInitialInstanceInfo(Accelerator* accel,Arena* out,Arena* temp,Array<Partition> partitions);
Array<Partition> GenerateInitialPartitions(Accelerator* accel,Arena* out);

void FillInstanceInfo(AccelInfoIterator initialIter,Arena* out,Arena* temp);

AccelInfo CalculateAcceleratorInfo(Accelerator* accel,bool recursive,Arena* out,Arena* temp);


// Array info related
// TODO: This should be built on top of AccelInfo (taking an iterator), instead of just taking the array of instance infos.
Array<Wire> ExtractAllConfigs(Array<InstanceInfo> info,Arena* out,Arena* temp);
Array<String> ExtractStates(Array<InstanceInfo> info,Arena* out);
Array<Pair<String,int>> ExtractMem(Array<InstanceInfo> info,Arena* out);

// TODO: Move this to Accelerator.hpp
void ShareInstanceConfig(FUInstance* instance, int shareBlockIndex);
void SetStatic(Accelerator* accel,FUInstance* instance);

String ReprStaticConfig(StaticId id,Wire* wire,Arena* out);

