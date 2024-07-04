#pragma once

#include <unordered_map>

#include "memory.hpp"

struct FUInstance;
struct Accelerator;
struct FUDeclaration;

// Strange forward declare
template<typename T> struct WireTemplate;
typedef WireTemplate<int> Wire;

struct StaticId{
   FUDeclaration* parent;
   String name;
};

template<> class std::hash<StaticId>{
   public:
   std::size_t operator()(StaticId const& s) const noexcept{
      std::size_t res = std::hash<String>()(s.name) + (std::size_t) s.parent;
      return (std::size_t) res;
   }
};

struct StaticData{
   Array<Wire> configs;
};

struct StaticInfo{
   StaticId id;
   StaticData data;
};

struct CalculatedOffsets{
   Array<int> offsets;
   int max;
};

enum MemType{
   CONFIG,
   STATE,
   DELAY,
   STATIC
};

enum NodeType{
  NodeType_UNCONNECTED,
  NodeType_SOURCE,
  NodeType_COMPUTE,
  NodeType_SINK,
  NodeType_SOURCE_AND_SINK
};

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
  FUDeclaration* parent;
  String fullName;
  bool isMergeMultiplexer;
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
  Array<int> inputDelay;
  Array<int> outputLatencies;
};

struct AccelInfo{
  Array<Array<InstanceInfo>> infos; // Should join names with infos
  Array<InstanceInfo> baseInfo;
  Array<String> names;
  Array<Array<int>> inputDelays;
  Array<Array<int>> outputDelays;
  
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

