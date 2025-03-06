
#pragma once

#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "utils.hpp"

static const int DELAY_SIZE = 7;

struct Accelerator;
struct InstanceInfo;
struct FUDeclaration;

// Type can differ because of Merge.
struct SingleTypeStructElement{
  String type;
  String name;
  int arraySize; // Zero or one represent same thing: no array.
};

struct TypeStructInfoElement{
  Array<SingleTypeStructElement> typeAndNames; // Because of config sharing, we can have multiple elements occupying the same position. Type can also differ because of merge.
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

struct IndexInfo{
  bool isPadding;
  int pos;
  int amountOfEntries;
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
  bool isStatic;
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

struct AddressGenDef;
extern Pool<AddressGenDef> addressGens;

struct StructInfo;

struct StructElement{
  StructInfo* childStruct; // If nulltpr, then leaf

  // TODO: We could put a Type* here, so that later we can handle different sizes (char,short,int,etc).
  //       Instead of just having childStruct point to a StructInfo("iptr");

  String name;
  int localPos; // Inside the struct info that contains this element
  int size;
  bool isMergeMultiplexer;
  bool doesNotBelong; // Leads to padding being added. From merge 
};

struct StructInfo{
  String name;
  String originalName; // NOTE: kinda wathever, mainly used to generate generic address gens.

  // As such, it appears that the easiest way of doing stuff is to store struct info inside the InstanceInfo and not the other way around.
  
  ArenaDoubleList<StructElement>* memberList;
};

size_t HashStructInfo(StructInfo* info);

template<> struct std::hash<StructElement>{
  std::size_t operator()(StructElement const& s) const noexcept{
    std::size_t res = std::hash<String>()(s.name) + s.size + s.localPos + (s.isMergeMultiplexer ? 1 : 0);

    if(s.childStruct != nullptr){
      res += HashStructInfo(s.childStruct);
    }
    
    return res;
   }
};

static bool operator==(StructInfo& l,StructInfo& r);

static bool operator==(StructElement& l,StructElement& r){
  bool res = (*l.childStruct == *r.childStruct &&
              l.name == r.name &&
              l.size == r.size &&
              l.localPos == r.localPos &&
              l.isMergeMultiplexer == r.isMergeMultiplexer);
  
  return res;
}

template<> struct std::hash<StructInfo>{
   std::size_t operator()(StructInfo const& s) const noexcept{
     std::size_t res = 0;
     for(DoubleLink<StructElement>* ptr = s.memberList ? s.memberList->head : nullptr; ptr; ptr = ptr->next){
       res += std::hash<StructElement>()(ptr->elem);
     }
     res += std::hash<String>()(s.name);
     return res;
   }
};

static bool operator==(StructInfo& l,StructInfo& r){
  if(!(l.name == r.name)){
    return false;
  }
  
  DoubleLink<StructElement>* lPtr = l.memberList ? l.memberList->head : nullptr;
  DoubleLink<StructElement>* rPtr = r.memberList ? r.memberList->head : nullptr;
  for(; lPtr && rPtr; lPtr = lPtr->next,rPtr = rPtr->next){
    if(!(lPtr->elem == rPtr->elem)){
      return false;
    }
  }

  return true;
}

Array<FUDeclaration*> SortTypesByMemDependency(Array<FUDeclaration*> types,Arena* out);

void OutputCircuitSource(FUDeclaration* decl,FILE* file);
void OutputIterativeSource(FUDeclaration* decl,FILE* file);
void OutputTopLevelFiles(Accelerator* accel,FUDeclaration* topLevelDecl,const char* hardwarePath,const char* softwarePath,bool isSimple);
