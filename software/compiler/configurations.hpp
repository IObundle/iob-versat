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
// This approach is very slow but easier to debug since everything related to one unit is all in the same place.
// Until I find a better way of debugging (visualizing) SoA, this will stay like this for a while.

// TODO: Put some note explaining the required changes when inserting stuff here.
struct InstanceInfo{
  int level;
  FUDeclaration* decl;
  FUDeclaration* parent;

  int id;
  String name;
  String baseName; // NOTE: If the unit does not belong to the merge partition the baseName will equal name.
  String fullName;
  
  Opt<int> globalStaticPos; // Separating static from global makes stuff simpler. If mixing together, do not forget that struct generation cares about source of configPos.
  Opt<int> globalConfigPos;
  Opt<int> localConfigPos;
  int isConfigStatic; // Static must be handle separately, for the top level accelerator. 
  int configSize;

  bool isStatic;
  bool isGloballyStatic;
  bool isShared;
  int sharedIndex;

  Opt<int> statePos;
  int stateSize;
  
  // Some of these could be removed and be computed from the others
  Opt<iptr> memMapped; // Used on Code Gen, to create the addresses for the memories.
  Opt<int> memMappedSize;
  Opt<int> memMappedBitSize;
  Opt<String> memDecisionMask; // This is local to the accelerator

  Opt<int> delayPos;
  Array<int> extraDelay;
  int baseNodeDelay;
  int delaySize;

  bool isComposite;
  bool isMerge;

  // Sepcific to merge muxs
  bool isMergeMultiplexer;
  int mergePort;
  int muxGroup;

  bool belongs;
  int special;
  int localOrder;
  FUInstance* inst;

  NodeType connectionType;
  Array<int> inputDelays;
  Array<int> outputLatencies;
  Array<int> portDelay;
  int partitionIndex; // What does this do?

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
};

// TODO: A lot of values are repeated between merge partitions and the like. Not a problem for now, maybe tackle it when things become stable. Or maybe leave it be, could be easier in future if we want to implement something more complex.
struct AccelInfo{
  Array<MergePartition> infos;
    
  int inputs;
  int outputs;

  int configs;
  int states;
  int delays;
  int nIOs;
  int statics;
  int staticBits;
  int sharedUnits;
  int externalMemoryInterfaces;
  int numberConnections;
  
  int memoryMappedBits;
  int unitsMapped;
  bool isMemoryMapped;
  bool signalLoop;
  bool implementsDone;
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
void FillAccelInfoFromCalculatedInstanceInfo(AccelInfo* info,Accelerator* accel);

Array<InstanceInfo*> GetAllSameLevelUnits(AccelInfo* info,int level,int mergeIndex,Arena* out);

// TODO: mergeIndex seems to be the wrong approach. Check the correct approach when trying to simplify merge.
Array<InstanceInfo> GenerateInitialInstanceInfo(Accelerator* accel,Arena* out,Array<Partition> partitions);
Array<Partition> GenerateInitialPartitions(Accelerator* accel,Arena* out);

void FillInstanceInfo(AccelInfoIterator initialIter,Arena* out);

// This function does not perform calculations that are only relevant to the top accelerator (like static units configs and such).
AccelInfo CalculateAcceleratorInfo(Accelerator* accel,bool recursive,Arena* out);

Array<int> ExtractInputDelays(AccelInfoIterator top,Arena* out);
Array<int> ExtractOutputLatencies(AccelInfoIterator top,Arena* out);

// Array info related
// TODO: This should be built on top of AccelInfo (taking an iterator), instead of just taking the array of instance infos.
Array<Wire> ExtractAllConfigs(Array<InstanceInfo> info,Arena* out);
Array<String> ExtractStates(Array<InstanceInfo> info,Arena* out);
Array<Pair<String,int>> ExtractMem(Array<InstanceInfo> info,Arena* out);

// TODO: Move this to Accelerator.hpp
void ShareInstanceConfig(FUInstance* instance, int shareBlockIndex);
void SetStatic(Accelerator* accel,FUInstance* instance);

String ReprStaticConfig(StaticId id,Wire* wire,Arena* out);

