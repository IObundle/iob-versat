#pragma once

#include <unordered_map>

#include "memory.hpp"

struct FUInstance;
struct InstanceNode;
struct OrderedInstance;
struct Accelerator;
struct FUDeclaration;

// Strange forward declare
template<typename T> struct WireTemplate;
typedef WireTemplate<int> Wire;

struct FUInstance;
struct Versat;

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
   int offset;
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
   STATIC,
   EXTRA
};

struct InstanceInfo{
  FUDeclaration* parentDeclaration;
  FUDeclaration* decl;
  String name;
  String fullName;
  Optional<int> configPos;
  int configSize;
  Optional<int> statePos;
  int stateSize;
  Optional<iptr> memMapped;
  Optional<int> memMappedSize;
  Optional<int> memMappedBitSize;
  Optional<String> memMappedMask;
  Optional<int> delayPos;
  Array<int> delay;
  int baseDelay;
  int delaySize;
  int level;
  bool isComposite;
  bool isStatic;
  bool isShared;
};

struct AcceleratorInfo{
  Array<InstanceInfo> info;
  int configSize;
  int stateSize;
  int delaySize;
  int delay;
  int memSize;
};

Array<InstanceInfo> TransformGraphIntoArray(Accelerator* accel,bool recursive,Arena* out,Arena* temp);

CalculatedOffsets CalculateConfigOffsetsIgnoringStatics(Accelerator* accel,Arena* out);
CalculatedOffsets CalculateConfigurationOffset(Accelerator* accel,MemType type,Arena* out);
CalculatedOffsets CalculateOutputsOffset(Accelerator* accel,int offset,Arena* out);

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

// Extract configurations named with the top level expected name (not module name)
//OrderedConfigurations ExtractOrderedConfigurationNames(Versat* versat,Accelerator* accel,Arena* out,Arena* temp);
//Array<Wire> OrderedConfigurationsAsArray(OrderedConfigurations configs,Arena* out);

Array<InstanceInfo> ExtractFromInstanceInfoSameLevel(Array<InstanceInfo> instanceInfo,int level,Arena* out);
Array<InstanceInfo> ExtractFromInstanceInfo(Array<InstanceInfo> instanceInfo,Arena* out);

void CheckSanity(Array<InstanceInfo> instanceInfo,Arena* temp);

void PrintConfigurations(FUDeclaration* type,Arena* temp);
