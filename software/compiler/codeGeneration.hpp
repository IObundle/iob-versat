
#pragma once

#include "verilogParsing.hpp"
#include "utils.hpp"

struct Accelerator;
struct InstanceInfo;
struct FUDeclaration;
struct ConfigurationInfo;

// Type can differ because of Merge.
struct SingleTypeStructElement{
  String type;
  String name;
  int arraySize; // Zero or one represent same thing: no array.
};

struct TypeStructInfoElement{
  Array<SingleTypeStructElement> typeAndNames; // Because of config sharing, we can have multiple elements occupying the same position.
};

struct TypeStructInfo{
  String name;
  Array<TypeStructInfoElement> entries;
};

struct Difference{
  int index;
  int newValue;
};

struct DifferenceArray{
  int oldIndex;
  int newIndex;

  Array<Difference> differences;
};

struct MuxInfo{
  int configIndex;
  int val;
  String name;
  InstanceInfo* info;
};

struct WireInformation{
  Wire wire;
  int addr;
  int configBitStart;
};

struct SubTypesInfo{
  FUDeclaration* type;
  FUDeclaration* mergeTop;
  ConfigurationInfo* info;
  bool isFromMerged;
  bool containsMerged;
};

struct SameMuxEntities{
  int configPos;
  InstanceInfo* info;
};

template<> struct std::hash<SameMuxEntities>{
   std::size_t operator()(SameMuxEntities const& s) const noexcept{
     std::size_t res = s.configPos;
     return res;
   }
};

static bool operator==(const SameMuxEntities i0,const SameMuxEntities i1){
  bool res = (i0.configPos == i1.configPos);
  return res;
}

template<> struct std::hash<SubTypesInfo>{
   std::size_t operator()(SubTypesInfo const& s) const noexcept{
     std::size_t res = std::hash<void*>()(s.type) + std::hash<void*>()(s.mergeTop) + std::hash<bool>()(s.isFromMerged) + std::hash<void*>()(s.info) + std::hash<bool>()(s.containsMerged);
     return res;
   }
};

static bool operator==(const SubTypesInfo i0,const SubTypesInfo i1){
  bool res = Memcmp(&i0,&i1,1);
  return res;
}

struct AddressGenDef;
extern Pool<AddressGenDef> savedAddressGen;


// In order to get the names, I need to associate a given member to a merge index.
// Since multiplexers can be shared accross multiple merge indexes, I need some map of some sorts.
// One multiplexer can be used by dozens of merge indexes.
enum StructInfoType{
  StructInfoType_UNION_WITH_MERGE_MULTIPLEXERS
};

struct StructElement{
  String name;
  String type;
  int pos;
  int size;
}; 

struct StructInfo{
  StructInfoType type;
  
  Array<StructElement> elements;
};

Array<FUDeclaration*> SortTypesByConfigDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp);
Array<FUDeclaration*> SortTypesByMemDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp);
Array<TypeStructInfoElement> GenerateStructFromType(FUDeclaration* decl,Arena* out,Arena* temp);

void OutputCircuitSource(FUDeclaration* decl,FILE* file,Arena* temp,Arena* temp2);
void OutputIterativeSource(FUDeclaration* decl,FILE* file,Arena* temp,Arena* temp2);
void OutputVerilatorWrapper(FUDeclaration* type,Accelerator* accel,String outputPath,Arena* temp,Arena* temp2);
void OutputVerilatorMake(String topLevelName,String versatDir,Arena* temp,Arena* temp2);
void OutputVersatSource(Accelerator* accel,const char* hardwarePath,const char* softwarePath,bool isSimple,Arena* temp,Arena* temp2);
