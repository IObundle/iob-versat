#pragma once

#include <unordered_map>

#include "memory.hpp"
#include "accelerator.hpp"

struct InstanceInfo{
  int level;
  FUDeclaration* decl;
  String name;
  String baseName;
  Opt<int> configPos;
  int isConfigStatic;
  int configSize;
  Opt<int> statePos;
  int stateSize;
  Opt<iptr> memMapped;
  Opt<int> memMappedSize;
  Opt<int> memMappedBitSize;
  Opt<String> memMappedMask;
  Opt<int> delayPos;
  Array<int> delay;
  int baseDelay;
  int delaySize;
  bool isComposite;
  bool isStatic;
  bool isShared;
  int sharedIndex;
  FUDeclaration* parent;
  String fullName;
  bool isMergeMultiplexer;
  int mergeMultiplexerId;
  bool belongs;
  int special;
  int order;
  NodeType connectionType;
  int id;
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
};

struct AccelInfo{
  Array<Array<InstanceInfo>> infos; // Should join names with infos
  Array<InstanceInfo> baseInfo;
  Array<String> names;
  Array<Array<int>> inputDelays;
  Array<Array<int>> outputDelays;
  Array<Array<int>> muxConfigs;
  
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

struct TypeAndNameOnly{
  String type;
  String name;
};

struct Partition{
  int value;
  int max;
};

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

