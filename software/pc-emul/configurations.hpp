#ifndef INCLUDED_VERSAT_CONFIGURATIONS_HPP
#define INCLUDED_VERSAT_CONFIGURATIONS_HPP

//#include "versatPrivate.hpp"
#include "memory.hpp"

struct ComplexFUInstance;
struct InstanceNode;
struct OrderedInstance;
struct Accelerator;
struct FUDeclaration;
struct Wire;

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
   PushPtr<iptr> config;
   PushPtr<int> state;
   PushPtr<int> delay;
   PushPtr<Byte> mem;
   PushPtr<int> outputs;
   PushPtr<int> storedOutputs;
   PushPtr<iptr> statics;
   PushPtr<Byte> extraData;
   PushPtr<int> externalMemory;
   //PushPtr<UnitDebugData> debugData;

   void Init(Accelerator* accel);
   void Init(Accelerator* topLevel,ComplexFUInstance* inst);

   void AssertEmpty(bool checkStatic = true);
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

void PopulateAccelerator(Accelerator* topLevel,Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,std::unordered_map<StaticId,StaticData>* staticMap);
void PopulateTopLevelAccelerator(Accelerator* accel);

#endif // INCLUDED_VERSAT_CONFIGURATIONS_HPP
