#ifndef INCLUDED_VERSAT_CONFIGURATIONS_HPP
#define INCLUDED_VERSAT_CONFIGURATIONS_HPP

#include <unordered_map>

#include "memory.hpp"

struct FUInstance;
struct InstanceNode;
struct OrderedInstance;
struct Accelerator;
struct FUDeclaration;

template<typename T> struct WireTemplate;
typedef WireTemplate<int> Wire;

struct FUInstance;
struct Versat;

struct SizedConfig{
   iptr* ptr;
   int size;
};

struct StaticId{
   FUDeclaration* parent;
   String name;
};

template<> class std::hash<StaticId>{
   public:
   std::size_t operator()(StaticId const& s) const noexcept{
      std::size_t res = std::hash<String>()(s.name);
      res += (std::size_t) s.parent;

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

struct UnitDebugData{
   int debugBreak;
};

class FUInstanceInterfaces{
public:
   // Config delay and statics share the same configuration space. They are separated here to facilitate accelerator population
   PushPtr<iptr> config;
   PushPtr<iptr> delay;
   PushPtr<iptr> statics;

   PushPtr<int> state;
   PushPtr<Byte> mem;
   PushPtr<int> outputs;
   PushPtr<int> storedOutputs;
   PushPtr<Byte> extraData;
   PushPtr<Byte> externalMemory;

   void Init(Accelerator* accel);
   void Init(Accelerator* topLevel,FUInstance* inst);

   void AssertEmpty();
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
   EXTRA,
   OUTPUT,
   STORED_OUTPUT
};

CalculatedOffsets CalculateConfigOffsetsIgnoringStatics(Accelerator* accel,Arena* out);
CalculatedOffsets CalculateConfigurationOffset(Accelerator* accel,MemType type,Arena* out);
CalculatedOffsets CalculateOutputsOffset(Accelerator* accel,int offset,Arena* out);
void AddOffset(CalculatedOffsets* offsets,int amount);

int GetConfigurationSize(FUDeclaration* decl,MemType type);

// These functions extract directly from the configuration pointer. No change is made, not even a check to see if unit contains any configuration at all
CalculatedOffsets ExtractConfig(Accelerator* accel,Arena* out); // Extracts offsets from a non FUDeclaration accelerator. Static configs are of the form >= N, where N is the size returned in CalculatedOffsets
CalculatedOffsets ExtractState(Accelerator* accel,Arena* out);
CalculatedOffsets ExtractDelay(Accelerator* accel,Arena* out);
CalculatedOffsets ExtractMem(Accelerator* accel,Arena* out);
CalculatedOffsets ExtractOutputs(Accelerator* accel,Arena* out);
CalculatedOffsets ExtractExtraData(Accelerator* accel,Arena* out);
CalculatedOffsets ExtractDebugData(Accelerator* accel,Arena* out);

Hashmap<String,SizedConfig>* ExtractNamedSingleConfigs(Accelerator* accel,Arena* out);
Hashmap<String,SizedConfig>* ExtractNamedSingleStates(Accelerator* accel,Arena* out);
Hashmap<String,SizedConfig>* ExtractNamedSingleMem(Accelerator* accel,Arena* arena);

void PopulateAccelerator(Accelerator* topLevel,Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,std::unordered_map<StaticId,StaticData>* staticMap);
void PopulateTopLevelAccelerator(Accelerator* accel);

void InitializeAccelerator(Versat* versat,Accelerator* accel,Arena* temp);

void SetDefaultConfiguration(FUInstance* instance);
void ShareInstanceConfig(FUInstance* instance, int shareBlockIndex);
void SetStatic(Accelerator* accel,FUInstance* instance);

String ReprStaticConfig(StaticId id,Wire* wire,Arena* out);

struct OrderedConfigurations{
   Array<Wire> configs;
   Array<Wire> statics;
   Array<Wire> delays;
};

// Extract configurations named with the top level expected name (not module name)
OrderedConfigurations ExtractOrderedConfigurationNames(Versat* versat,Accelerator* accel,Arena* out,Arena* temp);
Array<Wire> OrderedConfigurationsAsArray(OrderedConfigurations configs,Arena* out);

void PrintConfigurations(FUDeclaration* type);

#endif // INCLUDED_VERSAT_CONFIGURATIONS_HPP
